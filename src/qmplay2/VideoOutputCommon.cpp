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

#include <VideoOutputCommon.hpp>
#include <Functions.hpp>

#include <QCoreApplication>
#include <qevent.h>
#include <QWidget>

#include <cmath>

using namespace std;
using namespace placeholders;

VideoOutputCommon::VideoOutputCommon(bool yInverted)
    : m_yMultiplier(yInverted ? -1.0f : 1.0f)
{
    m_rotAnimation.setEasingCurve(QEasingCurve::OutQuint);
    m_rotAnimation.setDuration(1000.0);

    QObject::connect(&m_rotAnimation, &QVariantAnimation::valueChanged,
                     bind(&VideoOutputCommon::rotValueUpdated, this, _1));
}
VideoOutputCommon::~VideoOutputCommon()
{
}

QWidget *VideoOutputCommon::widget() const
{
    return m_widget;
}

void VideoOutputCommon::resetOffsets()
{
    m_videoOffset = m_osdOffset = QPointF();
}

bool VideoOutputCommon::setSphericalView(bool sphericalView)
{
    if (m_sphericalView == sphericalView)
        return false;

    const bool isBlankCursor = (m_widget->cursor().shape() == Qt::BlankCursor);
    m_sphericalView = sphericalView;
    if (m_sphericalView)
    {
        m_widget->setProperty("customCursor", (int)Qt::OpenHandCursor);
        if (!isBlankCursor)
            m_widget->setCursor(Qt::OpenHandCursor);
        m_rot = QPointF(90.0, 90.0);
    }
    else
    {
        m_widget->setProperty("customCursor", QVariant());
        if (!isBlankCursor)
            m_widget->setCursor(Qt::ArrowCursor);
        m_buttonPressed = false;
    }

    return true;
}

QSize VideoOutputCommon::getRealWidgetSize(bool lh) const
{
    const auto widgetSizeF = m_widget->devicePixelRatioF() * QSizeF(m_widget->size() + QSize(0, lh ? m_widget->property("loseHeight").toInt() : 0));
    return QSize(
        widgetSizeF.width(),
        widgetSizeF.height()
    );
}

QPointF VideoOutputCommon::getOsdOffset() const
{
    const auto realSize = getRealWidgetSize();
    const auto lhSize = getRealWidgetSize(true);
    return QPointF(
        m_osdOffset.x(),
        m_osdOffset.y() * lhSize.height() / realSize.height()
    );
}

void VideoOutputCommon::updateSizes(bool transpose)
{
    const auto size = getRealWidgetSize(true);

    const auto scaledSize = m_zoom * (transpose
        ? QSizeF(1.0, m_aRatio).scaled(size, Qt::KeepAspectRatio)
        : QSizeF(m_aRatio, 1.0).scaled(size, Qt::KeepAspectRatio)
    );
    m_scaledSize = QSize( // round down to int
        scaledSize.width(),
        scaledSize.height()
    );

    const auto subsScaledSize = m_zoom * QSizeF(m_aRatio, 1.0).scaled(size, Qt::KeepAspectRatio);
    const auto subsScaledPos = (size - subsScaledSize) / 2.0;
    m_subsRect.setRect( // round down to int
        subsScaledPos.width(),
        subsScaledPos.height(),
        subsScaledSize.width(),
        subsScaledSize.height()
    );
}
void VideoOutputCommon::updateMatrix()
{
    QSize widgetSize;

    m_matrix.setToIdentity();

    widgetSize = getRealWidgetSize();
    m_matrix.scale(
        static_cast<float>(m_scaledSize.width()) / static_cast<float>(widgetSize.width()),
        static_cast<float>(m_scaledSize.height()) / static_cast<float>(widgetSize.height())
    );
    if (!m_videoOffset.isNull())
    {
        m_matrix.translate(-m_videoOffset.x(), m_videoOffset.y() * m_yMultiplier);
    }
    if (int loseHeight = m_widget->property("loseHeight").toInt())
    {
        m_matrix.translate(0, -loseHeight * m_widget->devicePixelRatioF() * m_yMultiplier / m_scaledSize.height());
    }

    if (m_sphericalView)
    {
        widgetSize = getRealWidgetSize(true);
        m_matrix.scale(
            static_cast<float>(widgetSize.width())  / static_cast<float>(m_scaledSize.width()),
            static_cast<float>(widgetSize.height()) / static_cast<float>(m_scaledSize.height())
        );
        m_matrix.scale(1.0f, m_yMultiplier, 1.0f);
        m_matrix.perspective(68.0f, static_cast<float>(widgetSize.width()) / static_cast<float>(widgetSize.height()), 0.001f, 2.0f);
        m_matrix.translate(0.0f, 0.0f, qBound<float>(-1.0f, (m_zoom > 1.0f) ? log10(m_zoom) : m_zoom - 1.0f, 0.99f));
        m_matrix.rotate(m_rot.x(), 1.0f, 0.0f, 0.0f);
        m_matrix.rotate(m_rot.y(), 0.0f, 0.0f, 1.0f);
    }
}

void VideoOutputCommon::dispatchEvent(QEvent *e, QObject *p)
{
    switch (e->type())
    {
        case QEvent::MouseButtonPress:
            if (m_sphericalView)
                mousePress360((QMouseEvent *)e);
            else
                mousePress((QMouseEvent *)e);
            break;
        case QEvent::MouseButtonRelease:
            if (m_sphericalView)
                mouseRelease360((QMouseEvent *)e);
            else
                mouseRelease((QMouseEvent *)e);
            break;
        case QEvent::MouseMove:
            if (m_sphericalView)
                mouseMove360((QMouseEvent *)e);
            else
                mouseMove((QMouseEvent *)e);
            break;
        case QEvent::TouchBegin:
        case QEvent::TouchUpdate:
            m_canWrapMouse = false;
            // fallthrough
        case QEvent::TouchEnd:
        case QEvent::Gesture:
            // Pass gesture and touch event to the parent
            QCoreApplication::sendEvent(p, e);
            break;
        default:
            break;
    }
}

void VideoOutputCommon::mousePress(QMouseEvent *e)
{
    if (e->buttons() & Qt::LeftButton)
    {
        m_moveVideo = (e->modifiers() & Qt::ShiftModifier);
        m_moveOSD = (e->modifiers() & Qt::ControlModifier);
        if (m_moveVideo || m_moveOSD)
        {
            m_widget->setProperty("customCursor", (int)Qt::ArrowCursor);
            m_widget->setCursor(Qt::ClosedHandCursor);
            m_mousePos = e->pos();
        }
    }
}
void VideoOutputCommon::mouseMove(QMouseEvent *e)
{
    if ((m_moveVideo || m_moveOSD) && (e->buttons() & Qt::LeftButton))
    {
        const QPoint newMousePos = e->pos();
        const QPointF mouseDiff = m_mousePos - newMousePos;

        if (m_moveVideo)
        {
            const auto dpr = m_widget->devicePixelRatioF();
            m_videoOffset += QPointF(
                mouseDiff.x() * dpr * 2.0 / m_scaledSize.width(),
                mouseDiff.y() * dpr * 2.0 / m_scaledSize.height()
            );
        }
        if (m_moveOSD)
        {
            m_osdOffset += QPointF(
                mouseDiff.x() * 2.0 / m_widget->width(),
                mouseDiff.y() * 2.0 / m_widget->height()
            );
        }

        m_mousePos = newMousePos;

        m_matrixChangeFn();
    }
}
void VideoOutputCommon::mouseRelease(QMouseEvent *e)
{
    if ((m_moveVideo || m_moveOSD) && e->button() == Qt::LeftButton)
    {
        m_widget->unsetCursor();
        m_widget->setProperty("customCursor", QVariant());
        m_moveVideo = m_moveOSD = false;
    }
}

void VideoOutputCommon::mousePress360(QMouseEvent *e)
{
    if (e->buttons() & Qt::LeftButton)
    {
        m_widget->setCursor(Qt::ClosedHandCursor);
        m_mouseTime = Functions::gettime();
        m_buttonPressed = true;
        m_rotAnimation.stop();
        m_mousePos = e->pos();
    }
}
void VideoOutputCommon::mouseMove360(QMouseEvent *e)
{
    if (m_mouseWrapped)
    {
        m_mouseWrapped = false;
    }
    else if (m_buttonPressed && (e->buttons() & Qt::LeftButton))
    {
        const QPoint newMousePos = e->pos();
        const QPointF mouseDiff = QPointF(m_mousePos - newMousePos) / 10.0;

        m_rot.setX(qBound<qreal>(0.0, m_rot.rx() + mouseDiff.y(), 180.0));
        m_rot.ry() -= mouseDiff.x();

        const double currTime = Functions::gettime();
        const double mouseTimeDiff = qMax(currTime - m_mouseTime, 0.001);
        const QPointF movPerSec(mouseDiff.y() / mouseTimeDiff / 5.0, -mouseDiff.x() / mouseTimeDiff / 5.0);
        if (m_rotAnimation.state() != QAbstractAnimation::Stopped)
            m_rotAnimation.stop();
        m_rotAnimation.setEndValue(m_rot + movPerSec);
        m_mouseTime = currTime;

        m_mousePos = newMousePos;
        if (e->source() == Qt::MouseEventNotSynthesized)
        {
            m_mouseWrapped = m_canWrapMouse
                ? Functions::wrapMouse(m_widget, m_mousePos, 1)
                : (m_canWrapMouse = true)
            ;
        }

        m_matrixChangeFn();
    }
}
void VideoOutputCommon::mouseRelease360(QMouseEvent *e)
{
    if (m_buttonPressed && e->button() == Qt::LeftButton)
    {
        if ((Functions::gettime() - m_mouseTime) >= 0.075)
        {
            m_rotAnimation.stop();
        }
        else
        {
            m_rotAnimation.setStartValue(m_rot);
            m_rotAnimation.start();
        }
        m_widget->setCursor(Qt::OpenHandCursor);
        m_buttonPressed = false;
    }
}

void VideoOutputCommon::rotValueUpdated(const QVariant &value)
{
    if (m_buttonPressed)
        return;

    const auto newRot = value.toPointF();
    m_rot.setX(qBound<qreal>(0.0, newRot.x(), 180.0));
    m_rot.setY(newRot.y());

    m_matrixChangeFn();
}
