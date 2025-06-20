cmake_minimum_required(VERSION 3.16)
if(APPLE)
    project(QMPlay2 LANGUAGES CXX OBJCXX)
else()
    project(QMPlay2 LANGUAGES CXX)
endif()

if (NOT ${CMAKE_GENERATOR} MATCHES "Ninja")
    message(FATAL_ERROR "Unsupported generator ${CMAKE_GENERATOR}. Ninja is the only supported generator.")
endif()

if(WIN32)
    set(CUSTOM_INCLUDE_DIR "" CACHE STRING "Custom include directory")
    set(CUSTOM_LINK_DIR "" CACHE STRING "Custom link directory")

    if(CUSTOM_INCLUDE_DIR)
        set(CMAKE_INCLUDE_PATH ${CUSTOM_INCLUDE_DIR})
        include_directories(${CUSTOM_INCLUDE_DIR})
    endif()
    if(CUSTOM_LINK_DIR)
        set(CMAKE_LIBRARY_PATH ${CUSTOM_LINK_DIR})
        link_directories(${CUSTOM_LINK_DIR})
    endif()
endif()

# Set default build type as Release
if(NOT CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE STREQUAL "None")
    set(CMAKE_BUILD_TYPE "Release")
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/src/cmake/Modules")

include(FeatureSummary)

if(NOT CMAKE_HOST_WIN32 AND NOT ANDROID)
    find_package(PkgConfig REQUIRED)
else()
    find_package(DummyPkgConfig REQUIRED)
endif()

if((NOT DEFINED BUILD_WITH_QT6 OR BUILD_WITH_QT6) AND NOT DEFINED Qt6_FOUND AND (NOT DEFINED QT_QMAKE_EXECUTABLE OR QT_QMAKE_EXECUTABLE MATCHES "qmake6"))
    find_package(Qt6 6.1)
endif()
option(BUILD_WITH_QT6 "Build with Qt6" ${Qt6_FOUND})
add_feature_info(Qt6 BUILD_WITH_QT6 "Build with Qt6")

if(BUILD_WITH_QT6)
    set(QT_PREFIX Qt6)
    find_package(Qt6Widgets 6.1 REQUIRED)
    find_package(Qt6Core5Compat 6.1 REQUIRED)
else()
    set(QT_PREFIX Qt5)
    find_package(Qt5Widgets 5.15.2 REQUIRED)
endif()

pkg_check_modules(LIBAVFORMAT REQUIRED libavformat>=58.12.100)
pkg_check_modules(LIBAVCODEC REQUIRED libavcodec>=58.18.100)
pkg_check_modules(LIBSWSCALE REQUIRED libswscale>=5.1.100)
pkg_check_modules(LIBAVUTIL REQUIRED libavutil>=56.14.100)
pkg_check_modules(LIBSWRESAMPLE REQUIRED libswresample>=3.1.100)

set(CMAKE_LINK_DEPENDS_NO_SHARED ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOMOC_COMPILER_PREDEFINES OFF)

option(USE_PCH "Use precompiled headers" OFF)
add_feature_info(PCH USE_PCH "Use precompiled headers")

option(CMAKE_INTERPROCEDURAL_OPTIMIZATION "Enable link-time code generation" OFF)
add_feature_info("Interprocedural optimization" CMAKE_INTERPROCEDURAL_OPTIMIZATION "Enable link-time code generation")

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(DEFAULT_ALSA ON)
    set(DEFAULT_PORTAUDIO OFF)
elseif(ANDROID)
    set(DEFAULT_ALSA OFF)
    set(DEFAULT_PORTAUDIO OFF)
elseif(HAIKU)
    set(DEFAULT_ALSA OFF)
    set(DEFAULT_PORTAUDIO OFF)
else()
    set(DEFAULT_ALSA OFF)
    set(DEFAULT_PORTAUDIO ON)
endif()

if(APPLE OR ANDROID)
    set(DEFAULT_CUVID OFF)
else()
    set(DEFAULT_CUVID ON)
endif()

add_definitions(-D__STDC_CONSTANT_MACROS -D__STDC_LIMIT_MACROS -DQT_USE_QSTRINGBUILDER)

if (BUILD_WITH_QT6)
    add_definitions(-DQ_ENTER_EVENT=QEnterEvent)
else()
    add_definitions(-DQ_ENTER_EVENT=QEvent)
endif()

option(USE_UPDATES "Build with software updates" ON)
add_feature_info(Updates USE_UPDATES "Build with software updates")

if(WIN32)
    option(USE_CMD "Show CMD when running QMPlay2" OFF)
    mark_as_advanced(USE_CMD)
    add_feature_info(CMD USE_CMD "Show CMD when running QMPlay2")
endif()

option(USE_OPENGL "Build with OpenGL support" ON)
add_feature_info(OpenGL USE_OPENGL "Build with OpenGL support")
if(USE_OPENGL)
    add_definitions(-DUSE_OPENGL)
    if(BUILD_WITH_QT6)
        find_package(Qt6OpenGL REQUIRED)
        find_package(Qt6OpenGLWidgets REQUIRED)
    endif()
endif()

if(NOT APPLE)
    if (BUILD_WITH_QT6)
        get_target_property(${QT_PREFIX}GuiFeatures ${QT_PREFIX}::Gui QT_ENABLED_PUBLIC_FEATURES)
    else()
        get_target_property(${QT_PREFIX}GuiFeatures ${QT_PREFIX}::Gui INTERFACE_QT_ENABLED_FEATURES)
    endif()
    list(FIND ${QT_PREFIX}GuiFeatures "vulkan" ${QT_PREFIX}Vulkan)
    if(${${QT_PREFIX}Vulkan} GREATER -1)
        set(VULKAN_AVAILABLE ON)
    endif()
endif()
option(USE_VULKAN "Build with Vulkan support" ${VULKAN_AVAILABLE})
add_feature_info(Vulkan USE_VULKAN "Build with Vulkan support")
if(USE_VULKAN)
    add_definitions(-DUSE_VULKAN)
endif()

option(USE_GLSLC "Compile Vulkan shaders" OFF)
add_feature_info(GLSLC USE_GLSLC "Compile Vulkan shaders")

if(NOT WIN32 AND NOT APPLE AND NOT ANDROID AND NOT HAIKU)
    option(USE_FREEDESKTOP_NOTIFICATIONS "Use Freedesktop notifications" ON)
    add_feature_info("Freedesktop notifications" USE_FREEDESKTOP_NOTIFICATIONS "Use Freedesktop notifications")

    option(USE_DBUS_PM "Use D-Bus power management" ON)
    add_feature_info("Use D-Bus power management" USE_DBUS_PM "Use D-Bus power management")
endif()

option(USE_LIBASS "Build with libass support" ON)
add_feature_info(libass USE_LIBASS "Build with libass support")

if(NOT DEFINED USE_RUBBERBAND)
    pkg_check_modules(RUBBERBAND rubberband>=3.0.0)
endif()
option(USE_RUBBERBAND "Build with RubberBand support" ${RUBBERBAND_FOUND})
add_feature_info(RubberBand USE_RUBBERBAND "Build with RubberBand support")

option(USE_INPUTS "Build with Inputs module" ON)
add_feature_info(Inputs USE_INPUTS "Build with Inputs module")

option(USE_MODPLUG "Build with Modplug module" ON)
add_feature_info(Modplug USE_MODPLUG "Build with Modplug module")

option(USE_EXTENSIONS "Build with Extensions module" ON)
add_feature_info(Extensions USE_EXTENSIONS "Build with Extensions module")

if(USE_EXTENSIONS)
    option(USE_MEDIABROWSER "Build with MediaBrowser support" ON)
    add_feature_info(MediaBrowser USE_MEDIABROWSER "Build with MediaBrowser support")

    option(USE_LASTFM "Build with LastFM support" ON)
    add_feature_info(LastFM USE_LASTFM "Build with LastFM support")

    option(USE_LYRICS "Build with lyrics support" ON)
    add_feature_info(Lyrics USE_LYRICS "Build with lyrics support")

    option(USE_RADIO "Build with Radio Browser support" ON)
    add_feature_info(Radio USE_RADIO "Build with Radio Browser support")

    option(USE_YOUTUBE "Build with YouTube support" ON)
    add_feature_info(YouTube USE_YOUTUBE "Build with YouTube support")

    option(USE_OPENSUBTITLES "Build with OpenSubtitles support" ON)
    add_feature_info(OpenSubtitles USE_OPENSUBTITLES "Build with OpenSubtitles support")
endif()

if(USE_YOUTUBE OR USE_MEDIABROWSER)
    set(USE_YOUTUBEDL ON)
    add_definitions(-DUSE_YOUTUBEDL)
else()
    set(USE_YOUTUBEDL OFF)
endif()

if(USE_EXTENSIONS AND NOT WIN32 AND NOT APPLE AND NOT ANDROID AND NOT HAIKU)
    option(USE_MPRIS2 "Build Extensions with MPRIS2 support" ON)
    add_feature_info(MPRIS2 USE_MPRIS2 "Build Extensions with MPRIS2 support")
endif()

option(USE_VISUALIZATIONS "Build with Visualizations module" ON)
add_feature_info(Visualizations USE_VISUALIZATIONS "Build with Visualizations module")

option(USE_AUDIOFILTERS "Build with AudioFilters module" ON)
add_feature_info(AudioFilters USE_AUDIOFILTERS "Build with AudioFilters module")

if(USE_AUDIOFILTERS)
    pkg_check_modules(LIBAVFILTER libavfilter>=8.44.100)
    set(DEFAULT_LIBAVFILTER ${LIBAVFILTER_FOUND})

    option(USE_AVAUDIOFILTER "Build with AvAudioFilter support" ${DEFAULT_LIBAVFILTER})
    add_feature_info(AvAAudioFilter USE_AVAUDIOFILTER "Build with AvAudioFilter support")
endif()

option(USE_VIDEOFILTERS "Build with VideoFilters module" ON)
add_feature_info(VideoFilters USE_VIDEOFILTERS "Build with VideoFilters module")

option(USE_PORTAUDIO "Build with PortAudio module" ${DEFAULT_PORTAUDIO})
add_feature_info(PortAudio USE_PORTAUDIO "Build with PortAudio module")

if(NOT DEFINED USE_TAGLIB)
    find_package(TagLib)
endif()
option(USE_TAGLIB "Build with tags editor" ${TAGLIB_FOUND})
add_feature_info(TagLib USE_TAGLIB "Build with tags editor")

if(NOT DEFINED LIBAVUTIL_VERSION)
    set(LIBAVUTIL_VERSION "58.29.100")
endif()
if(NOT DEFINED USE_FFMPEG_VKVIDEO AND USE_VULKAN AND ${LIBAVUTIL_VERSION} VERSION_GREATER_EQUAL "58.29.100")
    set(DEFAULT_USE_FFMPEG_VKVIDEO ON)
endif()
option(USE_FFMPEG_VKVIDEO "Build VkVideo acceleration into FFmpeg" ${DEFAULT_USE_FFMPEG_VKVIDEO})
add_feature_info(VkVideo USE_FFMPEG_VKVIDEO "Build VkVideo acceleration into FFmpeg")

if(NOT DEFINED USE_FFMPEG_VAAPI)
    pkg_check_modules(LIBS_VAAPI libva>=1.1.0 libva-drm)
endif()
option(USE_FFMPEG_VAAPI "Build VAAPI acceleration into FFmpeg" ${LIBS_VAAPI_FOUND})
add_feature_info(VAAPI USE_FFMPEG_VAAPI "Build VAAPI acceleration into FFmpeg")

if(WIN32)
    option(USE_FFMPEG_DXVA2 "Build DXVA2 acceleration into FFmpeg" ON)
    add_feature_info(DXVA2 USE_FFMPEG_DXVA2 "Build DXVA2 acceleration into FFmpeg")

    if(USE_VULKAN)
        option(USE_FFMPEG_D3D11VA "Build D3D11VA acceleration into FFmpeg" ON)
        add_feature_info(DXVA2 USE_FFMPEG_D3D11VA "Build D3D11VA acceleration into FFmpeg")
    endif()
endif()

if(APPLE)
    option(USE_FFMPEG_VTB "Build VideoToolBox acceleration into FFmpeg" ON)
    add_feature_info(VTB USE_FFMPEG_VTB "Build VideoToolBox acceleration into FFmpeg")
endif()

if(NOT DEFINED USE_FFMPEG_AVDEVICE)
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        pkg_check_modules(LIBAVDEVICE libavdevice)
        set(DEFAULT_AVDEVICE ${LIBAVDEVICE_FOUND})
    else()
        set(DEFAULT_AVDEVICE OFF)
    endif()
endif()
option(USE_FFMPEG_AVDEVICE "Build FFmpeg with libavdevice suport" ${DEFAULT_AVDEVICE})
add_feature_info(libavdevice USE_FFMPEG_AVDEVICE "Build FFmpeg with libavdevice suport")

if(NOT DEFINED USE_AUDIOCD)
    pkg_check_modules(LIBCD libcdio libcddb)
endif()
option(USE_AUDIOCD "Build with AudioCD module" ${LIBCD_FOUND})
add_feature_info(AudioCD USE_AUDIOCD "Build with AudioCD module")

option(USE_ALSA "Build with ALSA module" ${DEFAULT_ALSA})
add_feature_info(ALSA USE_ALSA "Build with ALSA module")

if(NOT DEFINED USE_CHIPTUNE_GME)
    find_package(GME)
endif()
option(USE_CHIPTUNE_GME "Build Chiptune with GME support" ${LIBGME_FOUND})
add_feature_info("Chiptune GME" USE_CHIPTUNE_GME "Build Chiptune with GME support")

if(NOT DEFINED USE_CHIPTUNE_SID)
    pkg_check_modules(LIBSIDPLAYFP libsidplayfp)
endif()
option(USE_CHIPTUNE_SID "Build Chiptune with SIDPLAY support" ${LIBSIDPLAYFP_FOUND})
add_feature_info("Chiptune SIDPLAY" USE_CHIPTUNE_SID "Build Chiptune with SIDPLAY support")

if(NOT DEFINED USE_PULSEAUDIO)
    pkg_check_modules(LIBPULSE libpulse-simple)
endif()
option(USE_PULSEAUDIO "Build with PulseAudio module" ${LIBPULSE_FOUND})
add_feature_info(PulseAudio USE_PULSEAUDIO "Build with PulseAudio module")

if(NOT DEFINED USE_PIPEWIRE)
    pkg_check_modules(LIBPIPEWIRE libpipewire-0.3)
endif()
option(USE_PIPEWIRE "Build with PipeWire module" ${LIBPIPEWIRE_FOUND})
add_feature_info(PipeWire USE_PIPEWIRE "Build with PipeWire module")

if(NOT DEFINED USE_XVIDEO)
    pkg_check_modules(LIB_X11_XV x11 xv)
endif()
option(USE_XVIDEO "Build with XVideo module" ${LIB_X11_XV_FOUND})
add_feature_info(XVideo USE_XVIDEO "Build with XVideo module")

option(USE_CUVID "Build with CUVID module" ${DEFAULT_CUVID})
add_feature_info(CUVID USE_CUVID "Build with CUVID module")

option(USE_NOTIFY "Build additional notifications module" ON)
add_feature_info(Notifications USE_NOTIFY "Build additional notifications module")

set(LANGUAGES "All" CACHE STRING "A space-seperated list of translations to compile into QMPlay2 or \"All\"")

if(USE_FFMPEG_VAAPI)
    option(FIND_HWACCEL_DRIVERS_PATH "Find drivers path for hwaccel, useful for universal package" OFF)
    add_feature_info(FindHwaccelDrivers FIND_HWACCEL_DRIVERS_PATH "Find drivers path for hwaccel")
endif()

if(NOT WIN32)
    # get QMPlay2 version by running the version script
    execute_process(
        COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/version
        OUTPUT_VARIABLE QMPLAY2_VERSION
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    string(REPLACE "." "-" QMPLAY2_DATE ${QMPLAY2_VERSION})
    string(PREPEND QMPLAY2_DATE "20")
endif()

option(USE_GIT_VERSION "Append Git HEAD to QMPlay2 version (if exists)" ON)
if(USE_GIT_VERSION)
    # get QMPlay2 Git HEAD by running git command
    execute_process(
        COMMAND git rev-parse --short HEAD
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_VARIABLE QMPLAY2_GIT_HEAD
        OUTPUT_STRIP_TRAILING_WHITESPACE)
endif()
add_feature_info("Git version" QMPLAY2_GIT_HEAD "Append Git HEAD to QMPlay2 version")

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang") # GCC or Clang
    option(USE_ASAN "Use Address Sanitizer" OFF)
    add_feature_info("Address Sanitizer" USE_ASAN "Use Address Sanitizer")

    option(USE_UBSAN "Use Undefined Behavior Sanitizer" OFF)
    add_feature_info("Undefined Behavior Sanitizer" USE_UBSAN "Use Undefined Behavior Sanitizer")

    if(USE_ASAN)
        set(CMAKE_CXX_FLAGS "-fsanitize=address -fno-omit-frame-pointer ${CMAKE_CXX_FLAGS}")
        set(RELEASE_DEBUG_FLAG "-g")
    else()
        set(RELEASE_DEBUG_FLAG "-g0")
    endif()

    if(USE_UBSAN)
        set(CMAKE_CXX_FLAGS "-fsanitize=undefined ${CMAKE_CXX_FLAGS}")
    endif()

    # Hide symbols, and also make sure that debugging symbols are disabled
    set(CMAKE_CXX_FLAGS_RELEASE    "${RELEASE_DEBUG_FLAG} -fvisibility-inlines-hidden ${CMAKE_CXX_FLAGS_RELEASE}")
    set(CMAKE_CXX_FLAGS_MINSIZEREL "${RELEASE_DEBUG_FLAG} -fvisibility-inlines-hidden ${CMAKE_CXX_FLAGS_MINSIZEREL}")
    if(NOT USE_VULKAN)
        set(CMAKE_CXX_FLAGS_RELEASE    "-fno-exceptions -fvisibility=hidden ${CMAKE_CXX_FLAGS_RELEASE}")
        set(CMAKE_CXX_FLAGS_MINSIZEREL "-fno-exceptions -fvisibility=hidden ${CMAKE_CXX_FLAGS_MINSIZEREL}")
    endif()

    if(NOT CMAKE_BUILD_TYPE MATCHES "Deb")
        set(CMAKE_CXX_FLAGS "-Wno-deprecated-declarations ${CMAKE_CXX_FLAGS}") # disable deprecated declarations warnings for Release build
    endif()
    # enable warnings
    set(CMAKE_CXX_FLAGS "-Wall ${CMAKE_CXX_FLAGS}")

    if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL "15.0.0")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unqualified-std-cast-call")
    endif()

    if(CMAKE_MODULE_LINKER_FLAGS)
        # allow undefined references for modules
        string(REPLACE "-Wl,--no-undefined," "-Wl," CMAKE_MODULE_LINKER_FLAGS ${CMAKE_MODULE_LINKER_FLAGS})
        string(REPLACE "-Wl,--no-undefined" "" CMAKE_MODULE_LINKER_FLAGS ${CMAKE_MODULE_LINKER_FLAGS})
        string(REPLACE "--no-undefined," "" CMAKE_MODULE_LINKER_FLAGS ${CMAKE_MODULE_LINKER_FLAGS})
        string(REPLACE ",--no-undefined" "" CMAKE_MODULE_LINKER_FLAGS ${CMAKE_MODULE_LINKER_FLAGS})
        string(REPLACE "--no-undefined" "" CMAKE_MODULE_LINKER_FLAGS ${CMAKE_MODULE_LINKER_FLAGS})
    endif()

    if((NOT WIN32 AND NOT APPLE AND NOT ANDROID) AND CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fuse-ld=lld")
        set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -fuse-ld=lld")
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fuse-ld=lld")
    endif()
endif()

if(APPLE)
    set(MAC_BUNDLE_PATH "${CMAKE_INSTALL_PREFIX}/QMPlay2.app")

    # set paths to Bundle
    set(CMAKE_INSTALL_DATAROOTDIR "${MAC_BUNDLE_PATH}/Contents/share")
    set(CMAKE_INSTALL_LIBDIR      "${MAC_BUNDLE_PATH}/Contents/MacOS")
elseif(NOT WIN32)
    include(GNUInstallDirs)
endif()

if(NOT APPLE)
    set(INSTALL_PATH_ICONS "${CMAKE_INSTALL_DATAROOTDIR}/icons")
    set(INSTALL_PATH_MIME "${CMAKE_INSTALL_DATAROOTDIR}/mime")
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD" AND CMAKE_INSTALL_PREFIX STREQUAL "/usr/local")
    set(INSTALL_PATH_MAN "man") # exception for FreeBSD
else()
    set(INSTALL_PATH_MAN ${CMAKE_INSTALL_MANDIR})
endif()

if(APPLE)
    find_library(APPKIT_LIBRARY AppKit)
    find_library(IOKIT_LIBRARY IOKit)
    set(SOLID_ACTIONS_DEFAULT "None")
    set(DEFAULT_INSTALL_RPATH ON)
endif()

if(NOT WIN32 AND NOT APPLE)
    # RPATH
    option(SET_INSTALL_RPATH "Set RPATH for executable after install" ${DEFAULT_INSTALL_RPATH})

    # Solid actions
    set(SOLID_ACTIONS_INSTALL_PATH ${SOLID_ACTIONS_DEFAULT} CACHE STRING "The destination path for the Solid actions (\"None\" to disable)")
endif()

if(USE_FREEDESKTOP_NOTIFICATIONS OR USE_DBUS_PM OR USE_MPRIS2)
    find_package(${QT_PREFIX}DBus REQUIRED)
endif()

if(USE_MEDIABROWSER)
    find_package(${QT_PREFIX}Qml REQUIRED)
    set(USE_QML ON)
endif()

find_package(${QT_PREFIX}Svg REQUIRED)

if(USE_VULKAN)
    set(Vulkan_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/src/qmplay2/vulkan/headers/include")
    set(QMVK_USE_PCH ${USE_PCH})
    set(QMVK_WAIT_TIMEOUT_MS 2500)
    add_subdirectory(src/qmvk)
endif()
add_subdirectory(src/qmplay2)
add_subdirectory(src/modules)
add_subdirectory(src/gui)
if(LANGUAGES)
    add_subdirectory(lang)
endif()

if(WIN32)
    install(FILES AUTHORS ChangeLog LICENSE README.md DESTINATION ${CMAKE_INSTALL_PREFIX})
else()
    install(FILES AUTHORS ChangeLog LICENSE README.md DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/qmplay2")
endif()

if(NOT APPLE AND NOT WIN32)
    # uninstall target
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/src/cmake/Uninstall.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
        IMMEDIATE @ONLY)

    add_custom_target(uninstall
        COMMAND ${CMAKE_COMMAND} -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake")
endif()

feature_summary(WHAT ENABLED_FEATURES DESCRIPTION "Enabled features:")
feature_summary(WHAT DISABLED_FEATURES DESCRIPTION "Disabled features:")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
