cmake_minimum_required(VERSION 3.12 FATAL_ERROR)

set(PROJ detect)
PROJECT(${PROJ})

set(CMAKE_C_FLAGS "-Wall -pthread")
set(CMAKE_C_FLAGS_DEBUG "-g -O0")
set(CMAKE_C_FLAGS_RELEASE "-O3")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -std=c++14 -fPIC") #c++17 does not work with absl # do we need this flag? -> -lstdc++
set(CMAKE_CXX_FLAGS_DEBUG ${CMAKE_C_FLAGS_DEBUG})
set(CMAKE_CXX_FLAGS_RELEASE ${CMAKE_C_FLAGS_RELEASE})
#set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_subdirectory(libusb)
add_subdirectory(libedgetpu)

set(CMAKE_INSTALL_RPATH "$ORIGIN/.")
set(INCLUDE_DIRS
    .
    nzlogger/
    libedgetpu/
    libedgetpu/bazel-libedgetpu/bazel-out/k8-opt/bin/external/flatbuffers/src/_virtual_includes/flatbuffers/
    libedgetpu/tensorflow/
    libedgetpu/tflite/public/
    opencv/modules/core/include/
    )
set(LIBS usb udev edgetpu stdc++fs)

if ("${CMAKE_TOOLCHAIN_FILE}" STREQUAL "") # build for host
    set(INCLUDE_DIRS
        ${INCLUDE_DIRS}
        opencv/build/
        )
    set(LINK_DIRS
        ${CMAKE_SOURCE_DIR}/libedgetpu/out/direct/k8/
        ${CMAKE_SOURCE_DIR}/libedgetpu/libusb/build/
        ${CMAKE_SOURCE_DIR}/libedgetpu/bazel-build/
        /lib/x86_64-linux-gnu/
        ${CMAKE_SOURCE_DIR}/opencv/build/lib/
    )
else() # cross compile
    set(INCLUDE_DIRS
        ${INCLUDE_DIRS}
	opencv/build${arch}/
        MMALPP/
        userland/
        userland/build/lib
        )
    set(LINK_DIRS
        ${CMAKE_SOURCE_DIR}/libedgetpu/out/direct/rpi/
	${CMAKE_SOURCE_DIR}/libedgetpu/libusb/build${arch}/
        ${CMAKE_SOURCE_DIR}/libedgetpu/bazel-build/
        ${CMAKE_SOURCE_DIR}/lib/ #for libudev.so
	${CMAKE_SOURCE_DIR}/opencv/build${arch}/lib/
        ${CMAKE_SOURCE_DIR}/userland/build/lib
    )
    set(LIBS ${LIBS}
        EGL GLESv2 OpenVG WFC bcm_host brcmEGL brcmGLESv2 brcmOpenVG brcmWFC
        brcmjpeg containers debug_sym dtovl mmal mmal_components mmal_core mmal_omx
        mmal_omxutil mmal_util mmal_vc_client openmaxil vchiq_arm vcos vcsm
    )
    set(SOURCES
	picam/picam.cpp
	picam/picam.h
    )
    add_definitions(-DUSE_PICAM)
    set_source_files_properties(picam.cpp PROPERTIES CMAKE_CXX_FLAGS  -std=c++17)
endif()

set(SOURCES
    ${SOURCES}
    nzlogger/nzlogger.h
    nzlogger/nzlogger.cpp
    picam/camera_factory.cpp
    picam/camera_factory.h
    picam/camera_interface.h
    picam/fakecam.cpp
    picam/fakecam.h
    tfhelper/adapter.cc
    tfhelper/adapter.h
    tfhelper/bbox.h
    tfhelper/edgeTpuInterpreterBuilder.cpp
    tfhelper/edgeTpuInterpreterBuilder.h
    tfhelper/interpreterBuilderInterface.cpp
    tfhelper/interpreterBuilderInterface.h
    tfhelper/tfLiteInterpreter.cpp
    tfhelper/tfLiteInterpreter.h
    tfhelper/tfLiteInterpreterBuilder.cpp
    tfhelper/tfLiteInterpreterBuilder.h
    )

##########################################################################
add_executable(${PROJ} ${SOURCES} main.cc)
target_include_directories(${PROJ} PRIVATE
    ${INCLUDE_DIRS}
    opencv/modules/highgui/include/
    opencv/modules/imgcodecs/include/
    opencv/modules/imgproc/include/
    opencv/modules/videoio/include/
    )
target_link_directories(${PROJ} PRIVATE ${LINK_DIRS})
target_link_libraries(${PROJ} PRIVATE ${LIBS}
    opencv_video opencv_objdetect opencv_highgui opencv_imgcodecs opencv_dnn
    opencv_imgproc opencv_ml opencv_calib3d opencv_videoio opencv_core
    opencv_photo opencv_stitching opencv_features2d opencv_gapi opencv_flann
    )

##########################################################################
configure_file("${CMAKE_SOURCE_DIR}/download-models.sh" "${CMAKE_BINARY_DIR}/download-models.sh" COPYONLY)
