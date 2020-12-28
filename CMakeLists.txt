cmake_minimum_required(VERSION 3.12 FATAL_ERROR)

set(PROJ minimal)
PROJECT(${PROJ})

set(CMAKE_C_FLAGS "-Wall -pthread")
set(CMAKE_C_FLAGS_DEBUG "-g -O0")
set(CMAKE_C_FLAGS_RELEASE "-O3")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -std=c++14 -lstdc++ -fPIC") #c++17 does not work with absl
set(CMAKE_CXX_FLAGS_DEBUG ${CMAKE_C_FLAGS_DEBUG})
set(CMAKE_CXX_FLAGS_RELEASE ${CMAKE_C_FLAGS_RELEASE})
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_subdirectory(libusb)
add_subdirectory(libedgetpu)

set(CMAKE_INSTALL_RPATH "$ORIGIN/.")
set(INCLUDE_DIRS
    .
    libedgetpu/
    libedgetpu/tflite/public/
    libedgetpu/tensorflow/
    libedgetpu/bazel-libedgetpu/bazel-out/k8-opt/bin/external/flatbuffers/src/_virtual_includes/flatbuffers/
    opencv/modules/core/include/
    )
if ("${CMAKE_TOOLCHAIN_FILE}" STREQUAL "")
    set(INCLUDE_DIRS
        ${INCLUDE_DIRS}
        opencv/build/
        )
    set(LINK_DIRS
        ${CMAKE_SOURCE_DIR}/libedgetpu/out/direct/k8/
        ${CMAKE_SOURCE_DIR}/libedgetpu/libusb/build/
        ${CMAKE_SOURCE_DIR}/libedgetpu/bazel-build/
        /lib/x86_64-linux-gnu/
    )
else()
    set(INCLUDE_DIRS
        ${INCLUDE_DIRS}
        opencv/buildpi/
        )
    set(LINK_DIRS
        ${CMAKE_SOURCE_DIR}/libedgetpu/out/direct/rpi/
        ${CMAKE_SOURCE_DIR}/libedgetpu/libusb/buildpi/
        ${CMAKE_SOURCE_DIR}/libedgetpu/bazel-build/
        ${CMAKE_SOURCE_DIR}/lib/ #for libudev.so
    )
endif()
set(LIBS usb udev edgetpu)
set(SOURCES
    example/EdgeTpuInterpreterBuilder.cpp
    example/EdgeTpuInterpreterBuilder.h
    example/InterpreterBuilderInterface.cpp
    example/InterpreterBuilderInterface.h
    example/TfLiteInterpreter.cpp
    example/TfLiteInterpreter.h
    example/TfLiteInterpreterBuilder.cpp
    example/TfLiteInterpreterBuilder.h
    example/adapter.cc
    example/adapter.h
    example/bbox.h
    )
##########################################################################
add_executable(${PROJ} main.cc)
target_include_directories(${PROJ} PRIVATE ${INCLUDE_DIRS})
target_link_directories(${PROJ} PRIVATE ${LINK_DIRS})
target_link_libraries(${PROJ} PRIVATE ${LIBS})

##########################################################################
add_executable(classify ${SOURCES} example/classify.cc)
target_include_directories(classify PRIVATE ${INCLUDE_DIRS})
target_link_directories(classify PRIVATE ${LINK_DIRS})
target_link_libraries(classify PRIVATE ${LIBS})

##########################################################################
add_executable(detect ${SOURCES} example/detect.cc)
target_include_directories(detect PRIVATE ${INCLUDE_DIRS})
target_link_directories(detect PRIVATE ${LINK_DIRS})
target_link_libraries(detect PRIVATE ${LIBS})

##########################################################################
configure_file("${CMAKE_SOURCE_DIR}/download-models.sh" "${CMAKE_BINARY_DIR}/download-models.sh" COPYONLY)
