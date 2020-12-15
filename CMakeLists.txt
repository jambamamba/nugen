#mkdir buildPi
#cd buildPi
#cmake -DCMAKE_TOOLCHAIN_FILE=/home/dev/oosman/pi/Toolchain-RaspberryPi.cmake ../
#make VERBOSE=1 -j8
#https://www.pyimagesearch.com/2019/04/22/getting-started-with-google-corals-tpu-usb-accelerator/
#on raspi0: sudo apt-get install python3-edgetpu
#https://coral.ai/models/

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


add_subdirectory(libedgetpu/libusb)
add_subdirectory(libedgetpu)

add_executable(${PROJ}
    main.cc
)
target_include_directories(${PROJ} PRIVATE
    .
    libedgetpu/
    libedgetpu/tflite/public/
    libedgetpu/tensorflow/
    libedgetpu/bazel-libedgetpu/bazel-out/k8-opt/bin/external/flatbuffers/src/_virtual_includes/flatbuffers/
    )
target_link_directories(${PROJ} PUBLIC
    ${CMAKE_SOURCE_DIR}/libedgetpu/out/direct/k8/
    ${CMAKE_SOURCE_DIR}/libedgetpu/libusb/build/
    ${CMAKE_SOURCE_DIR}/libedgetpu/bazel-build/
    ${CMAKE_SOURCE_DIR}/libedgetpu/libusb/build
    /lib/x86_64-linux-gnu/
    )
target_link_libraries(${PROJ}
    usb
    udev
    edgetpu
)
