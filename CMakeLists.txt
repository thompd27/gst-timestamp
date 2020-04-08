cmake_minimum_required(VERSION 3.2)
project(gst-timestamp)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GST REQUIRED gstreamer-1.0>=1.4
                               gstreamer-sdp-1.0>=1.4
                               gstreamer-video-1.0>=1.4
                               gstreamer-app-1.0>=1.4)

pkg_check_modules(GSTLIBS REQUIRED
        gobject-2.0
        glib-2.0
        gstreamer-webrtc-1.0
        gstreamer-sdp-1.0
        gstreamer-pbutils-1.0
        libsoup-2.4
        json-glib-1.0
        gstreamer-rtsp-server-1.0
        gstreamer-check-1.0)

include_directories(${GST_INCLUDE_DIRS})
link_directories(${GST_LIBRARY_DIRS})
add_library(libgsttimestamp SHARED gst-timestamp.cpp gst-timestamp.h)
target_link_libraries(libgsttimestamp ${GST_LIBRARIES})

ADD_CUSTOM_COMMAND (
TARGET libgsttimestamp
POST_BUILD
COMMAND ${CMAKE_COMMAND}
ARGS -E copy ${CMAKE_CURRENT_BINARY_DIR}/Release/libgsttimestamp.dll ${GST_LIBRARY_DIRS}/gstreamer-1.0
)