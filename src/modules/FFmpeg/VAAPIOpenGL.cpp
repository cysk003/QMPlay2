/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2025  Błażej Szczygieł

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published
    by the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <VAAPIOpenGL.hpp>
#include <FFCommon.hpp>

#include <QMPlay2Core.hpp>
#include <Frame.hpp>

#include <QOpenGLContext>

#include <unistd.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>

#ifndef EGL_DMA_BUF_PLANE0_MODIFIER_LO_EXT
#    define EGL_DMA_BUF_PLANE0_MODIFIER_LO_EXT 0x3443
#endif
#ifndef EGL_DMA_BUF_PLANE0_MODIFIER_HI_EXT
#    define EGL_DMA_BUF_PLANE0_MODIFIER_HI_EXT 0x3444
#endif

struct VAAPIOpenGL::EGL
{
    EGLDisplay eglDpy = EGL_NO_DISPLAY;
    PFNEGLCREATEIMAGEKHRPROC eglCreateImageKHR = nullptr;
    PFNEGLDESTROYIMAGEKHRPROC eglDestroyImageKHR = nullptr;
    PFNGLEGLIMAGETARGETTEXTURE2DOESPROC glEGLImageTargetTexture2DOES = nullptr;
};

VAAPIOpenGL::VAAPIOpenGL()
    : m_egl(std::make_unique<EGL>())
{}
VAAPIOpenGL::~VAAPIOpenGL()
{
    clearSurfaces(false);
}

QString VAAPIOpenGL::name() const
{
    return VAAPIWriterName;
}

VAAPIOpenGL::Format VAAPIOpenGL::getFormat() const
{
    return NV12;
}

bool VAAPIOpenGL::isCopy() const
{
    return false;
}

bool VAAPIOpenGL::init(const int *widths, const int *heights, const SetTextureParamsFn &setTextureParamsFn)
{
    for (int p = 0; p < m_numPlanes; ++p)
    {
        if (m_widths[p] != widths[p] || m_heights[p] != heights[p])
        {
            clearTextures();
            for (int p = 0; p < m_numPlanes; ++p)
            {
                m_widths[p] = widths[p];
                m_heights[p] = heights[p];
            }
            glGenTextures(m_numPlanes, m_textures);
            break;
        }
    }

    for (int p = 0; p < m_numPlanes; ++p)
        setTextureParamsFn(m_textures[p]);

    if (m_egl->eglDpy != EGL_NO_DISPLAY && m_egl->eglCreateImageKHR && m_egl->eglDestroyImageKHR && m_egl->glEGLImageTargetTexture2DOES)
        return true;

    const auto context = QOpenGLContext::currentContext();
    if (!context)
    {
        QMPlay2Core.logError("VA-API :: Unable to get OpenGL context");
        m_error = true;
        return false;
    }

    m_egl->eglDpy = eglGetCurrentDisplay();
    if (!m_egl->eglDpy)
    {
        QMPlay2Core.logError("VA-API :: Unable to get EGL display");
        m_error = true;
        return false;
    }

    const auto extensionsRaw = eglQueryString(m_egl->eglDpy, EGL_EXTENSIONS);
    if (!extensionsRaw)
    {
        QMPlay2Core.logError("VA-API :: Unable to get EGL extensions");
        m_error = true;
        return false;
    }

    const auto extensions = QByteArray::fromRawData(extensionsRaw, qstrlen(extensionsRaw));
    if (!extensions.contains("EGL_EXT_image_dma_buf_import"))
    {
        QMPlay2Core.logError("VA-API :: EGL_EXT_image_dma_buf_import extension is not available");
        m_error = true;
        return false;
    }

    m_egl->eglCreateImageKHR = (PFNEGLCREATEIMAGEKHRPROC)context->getProcAddress("eglCreateImageKHR");
    m_egl->eglDestroyImageKHR = (PFNEGLDESTROYIMAGEKHRPROC)context->getProcAddress("eglDestroyImageKHR");
    m_egl->glEGLImageTargetTexture2DOES = (PFNGLEGLIMAGETARGETTEXTURE2DOESPROC)context->getProcAddress("glEGLImageTargetTexture2DOES");
    if (!m_egl->eglCreateImageKHR || !m_egl->eglDestroyImageKHR || !m_egl->glEGLImageTargetTexture2DOES)
    {
        QMPlay2Core.logError("VA-API :: Unable to get EGL function pointers");
        m_error = true;
        return false;
    }

    m_hasDmaBufImportModifiers = extensions.contains("EGL_EXT_image_dma_buf_import_modifiers");

    return true;
}

void VAAPIOpenGL::clear()
{
    m_egl->eglDpy = EGL_NO_DISPLAY;

    m_egl->eglCreateImageKHR = nullptr;
    m_egl->eglDestroyImageKHR = nullptr;
    m_egl->glEGLImageTargetTexture2DOES = nullptr;

    m_hasDmaBufImportModifiers = false;
    clearTextures();
}

bool VAAPIOpenGL::mapFrame(Frame &videoFrame)
{
    if (!videoFrame.isHW())
        return false;

    std::lock_guard<std::mutex> locker(m_mutex);

    if (m_availableSurfaces.count(videoFrame.hwData()) == 0)
        return false;

    VASurfaceID id;
    int vaField = videoFrame.isInterlaced()
        ? (videoFrame.isTopFieldFirst() != videoFrame.isSecondField())
            ? VA_TOP_FIELD
            : VA_BOTTOM_FIELD
        : VA_FRAME_PICTURE
    ;
    if (!m_vaapi->filterVideo(videoFrame, id, vaField))
        return false;

    auto &vaSurfaceDescr = m_surfaces[id];
    if (vaSurfaceDescr.fourcc == 0 && vaSurfaceDescr.num_objects == 0)
    {
        if (m_vaapi->m_mutex)
            m_vaapi->m_mutex->lock();
        if (vaExportSurfaceHandle(
                m_vaapi->VADisp,
                id,
                VA_SURFACE_ATTRIB_MEM_TYPE_DRM_PRIME_2,
                VA_EXPORT_SURFACE_READ_ONLY | VA_EXPORT_SURFACE_SEPARATE_LAYERS,
                &vaSurfaceDescr
            ) != VA_STATUS_SUCCESS)
        {
            if (m_vaapi->m_mutex)
                m_vaapi->m_mutex->unlock();
            QMPlay2Core.logError("VA-API :: Unable to export surface handle");
            m_surfaces.erase(id);
            m_error = true;
            return false;
        }
        if (m_vaapi->m_mutex)
            m_vaapi->m_mutex->unlock();
    }

    auto eraseSurface = [&] {
        m_surfaces.erase(id);
        closeFDs(vaSurfaceDescr);
    };

    const auto syncRet = vaSyncSurface(m_vaapi->VADisp, id);
    if (syncRet != VA_STATUS_SUCCESS)
    {
        eraseSurface();
        if (syncRet != VA_STATUS_ERROR_INVALID_CONTEXT)
        {
            QMPlay2Core.logError("VA-API :: Unable to sync surface");
            m_error = true;
        }
        return false;
    }

    for (uint32_t p = 0; p < vaSurfaceDescr.num_layers; ++p)
    {
        const auto &layer = vaSurfaceDescr.layers[p];
        const auto &object = vaSurfaceDescr.objects[layer.object_index[0]];

        EGLint attribs[] = {
            EGL_LINUX_DRM_FOURCC_EXT, (EGLint)layer.drm_format,
            EGL_WIDTH, (EGLint)videoFrame.width(p),
            EGL_HEIGHT, (EGLint)videoFrame.height(p),
            EGL_DMA_BUF_PLANE0_FD_EXT, object.fd,
            EGL_DMA_BUF_PLANE0_OFFSET_EXT, (EGLint)layer.offset[0],
            EGL_DMA_BUF_PLANE0_PITCH_EXT, (EGLint)layer.pitch[0],
            EGL_NONE, 0,
            EGL_NONE, 0,
            EGL_NONE
        };
        if (m_hasDmaBufImportModifiers)
        {
            attribs[12] = EGL_DMA_BUF_PLANE0_MODIFIER_LO_EXT;
            attribs[13] = static_cast<EGLint>(object.drm_format_modifier);
            attribs[14] = EGL_DMA_BUF_PLANE0_MODIFIER_HI_EXT;
            attribs[15] = static_cast<EGLint>(object.drm_format_modifier >> 32);
        }

        const auto image = m_egl->eglCreateImageKHR(
            m_egl->eglDpy,
            EGL_NO_CONTEXT,
            EGL_LINUX_DMA_BUF_EXT,
            nullptr,
            attribs
        );
        if (!image)
        {
            QMPlay2Core.logError("VA-API :: Unable to create EGL image");
            eraseSurface();
            m_error = true;
            return false;
        }

        glBindTexture(GL_TEXTURE_2D, m_textures[p]);
        m_egl->glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, image);

        m_egl->eglDestroyImageKHR(m_egl->eglDpy, image);
    }

    return true;
}

quint32 VAAPIOpenGL::getTexture(int plane)
{
    return m_textures[plane];
}

Frame VAAPIOpenGL::getCpuFrame(const Frame &videoFrame)
{
    Frame cpuFrame;
    const auto vppId = m_vaapi->getVppId();
    if (vppId != VA_INVALID_ID)
    {
        auto tmpFrame = videoFrame;
        const auto dataArr = tmpFrame.dataArr();
        const auto idBackup = dataArr[3];
        reinterpret_cast<quintptr &>(dataArr[3]) = vppId;
        cpuFrame = tmpFrame.downloadHwData();
        dataArr[3] = idBackup;
    }
    else
    {
        cpuFrame = videoFrame.downloadHwData();
    }
    return cpuFrame;
}

void VAAPIOpenGL::clearSurfaces(bool lock)
{
    if (lock)
        m_mutex.lock();
    for (auto &&s : m_surfaces)
        closeFDs(s.second);
    m_availableSurfaces.clear();
    m_surfaces.clear();
    if (lock)
        m_mutex.unlock();
}

void VAAPIOpenGL::insertAvailableSurface(uintptr_t id)
{
    std::lock_guard<std::mutex> locker(m_mutex);
    m_availableSurfaces.insert(id);
}

void VAAPIOpenGL::clearTextures()
{
    glDeleteTextures(m_numPlanes, m_textures);
    memset(m_textures, 0, sizeof(m_textures));
    memset(m_widths, 0, sizeof(m_widths));
    memset(m_heights, 0, sizeof(m_heights));
}

void VAAPIOpenGL::closeFDs(const VADRMPRIMESurfaceDescriptor &vaSurfaceDescr)
{
    for (uint32_t o = 0; o < vaSurfaceDescr.num_objects; ++o)
        ::close(vaSurfaceDescr.objects[o].fd);
}
