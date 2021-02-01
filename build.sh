#!/bin/bash -xe
set -xe

export CC=/usr/bin/gcc
export CXX=/usr/bin/g++

function parseArgs()
{
  for change in $@; do
      set -- `echo $change | tr '=' ' '`
      echo "variable name == $1  and variable value == $2"
      #can assign value to a variable like below
      eval $1=$2;
  done
}

function setupToolchainVars()
{
    parseArgs "$@"

    if [ "$arch" == "rpi0" ]; then
        eval TOOLCHAIN_ROOT_DIR=${HOME}/${DOCKERUSER}/.leila/toolchains/raspberrypi0
        eval STRIP_TOOL="$TOOLCHAIN_ROOT_DIR/x-tools/arm-rpi-linux-gnueabihf/bin/arm-rpi-linux-gnueabihf-strip"
        eval CXX_TOOL="$TOOLCHAIN_ROOT_DIR/x-tools/arm-rpi-linux-gnueabihf/bin/arm-rpi-linux-gnueabihf-g++"
    elif [ "$arch" == "rpi4" ]; then
        eval TOOLCHAIN_ROOT_DIR=${HOME}/${DOCKERUSER}/.leila/toolchains/raspberrypi4
	eval STRIP_TOOL="$TOOLCHAIN_ROOT_DIR/x-tools/aarch64-rpi4-linux-gnu/bin/aarch64-rpi4-linux-gnu-strip"
	eval CXX_TOOL="$TOOLCHAIN_ROOT_DIR/x-tools/aarch64-rpi4-linux-gnu/bin/aarch64-rpi4-linux-gnu-g++"
    elif [ "$arch" == "a53" ]; then
        eval TOOLCHAIN_ROOT_DIR=${HOME}/${DOCKERUSER}/.leila/toolchains/arm53sdk
        eval STRIP_TOOL="${TOOLCHAIN_ROOT_DIR}/sysroots/x86_64-pokysdk-linux/usr/bin/aarch64-poky-linux/aarch64-poky-linux-strip"
        eval CXX_TOOL="${TOOLCHAIN_ROOT_DIR}/sysroots/x86_64-pokysdk-linux/usr/bin/aarch64-poky-linux/aarch64-poky-linux-g++"
    fi
}

function cloneRepos()
{
    if [ ! -d libusb ]; then
       git clone https://github.com/jambamamba/libusb.git
    fi
    if [ ! -d systemd ]; then
       git clone https://github.com/jambamamba/systemd.git
    fi
    if [ ! -d libedgetpu ]; then
       git clone https://github.com/jambamamba/libedgetpu.git
    fi
    if [ ! -d tensorflow ]; then
       git clone https://github.com/jambamamba/tensorflow.git
    fi
    if [ ! -d opencv ]; then
       git clone https://github.com/opencv/opencv.git
    fi
    if [ ! -d curl ]; then
       git clone https://github.com/curl/curl.git
    fi
	if [ ! -d MMALPP ]; then
		git clone https://github.com/jambamamba/MMALPP.git
	fi
	if [ ! -d userland ]; then
		git clone https://github.com/raspberrypi/userland.git
	fi
}

function buildlibusb()
{
    parseArgs "$@"

    pushd libusb
        git checkout x86_64 #works for both host and rpi!
        if [ ! -f config.h ]; then
                ./autogen.sh
        fi

        if [ "$arch" == "host" ]; then
            mkdir -p build
            pushd build
                    if [ "$clean" == "true" ]; then rm -fr *;fi
                    cmake ..
                    make -j$(getconf _NPROCESSORS_ONLN) VERBOSE=1
            popd
        else
            mkdir -p "build${arch}"
            pushd "build${arch}"
                    if [ "$clean" == "true" ]; then rm -fr *;fi
                    cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=$TOOLCHAIN_ROOT_DIR/Toolchain.cmake ../
                    make -j$(getconf _NPROCESSORS_ONLN) VERBOSE=1
            popd
        fi
    popd
}

function buildlibcurl()
{
    parseArgs "$@"

    pushd curl
        if [ "$arch" == "host" ]; then
            mkdir -p build
            pushd build
                    if [ "$clean" == "true" ]; then rm -fr *;fi
                    cmake ..
                    make -j$(getconf _NPROCESSORS_ONLN) VERBOSE=1
            popd
        else
            mkdir -p "build${arch}"
            pushd "build${arch}"
                    if [ "$clean" == "true" ]; then rm -fr *;fi
                    #cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=$TOOLCHAIN_ROOT_DIR/Toolchain.cmake ../
                    #make -j$(getconf _NPROCESSORS_ONLN) VERBOSE=1
            popd
        fi
    popd
}

function buildopencv()
{
    parseArgs "$@"

    pushd opencv
        if [ "$arch" == "host" ]; then
            mkdir -p build
            pushd build
                    if [ "$clean" == "true" ]; then rm -fr *;fi
                    cmake -DOPENCV_FORCE_3RDPARTY_BUILD=ON ..
                    make -j$(getconf _NPROCESSORS_ONLN) VERBOSE=1
            popd
        else
            mkdir -p "build${arch}"
            pushd "build${arch}"
                    if [ "$clean" == "true" ]; then rm -fr *;fi
		    if [ "$arch" == "rpi0" ]; then
                        cmake -DCMAKE_BUILD_TYPE=Release -DOPENCV_FORCE_3RDPARTY_BUILD=ON -DCMAKE_TOOLCHAIN_FILE=$TOOLCHAIN_ROOT_DIR/Toolchain.cmake ../
			elif [ "$arch" == "rpi4" ] || [ "$arch" == "a53" ]; then
                        cmake -DCMAKE_BUILD_TYPE=Release -DOPENCV_FORCE_3RDPARTY_BUILD=ON -DPNG_ARM_NEON_OPT=0 -DCMAKE_TOOLCHAIN_FILE=$TOOLCHAIN_ROOT_DIR/Toolchain.cmake ../
                    fi
                    make -j$(getconf _NPROCESSORS_ONLN) VERBOSE=1
            popd
        fi
    popd
}

function buildlibcap()
{
    pushd /home/dev/oosman/.leila/toolchains/raspberrypi4/x-tools/aarch64-rpi4-linux-gnu/aarch64-rpi4-linux-gnu/sysroot/usr/include/sys
    sudo ln -s /home/dev/oosman/repos/nugen/libcap/libcap/include/sys/capability.h .
    sudo ln -s /home/dev/oosman/repos/nugen/libcap/libcap/include/sys/securebits.h .
    popd
    pushd /home/dev/oosman/.leila/toolchains/raspberrypi4/x-tools/aarch64-rpi4-linux-gnu/aarch64-rpi4-linux-gnu/sysroot/usr/lib
    sudo ln -s /home/dev/oosman/repos/nugen/libcap/libcap/libcap.so .
    popd
}

function buildlibmount()
{
    git clean -dxf
    ./autogen.sh
    export ac_cs_linux_vers=4
    export CFLAGS=
    export CPPFLAGS=
    export CC=/home/dev/oosman/.leila/toolchains/raspberrypi4/x-tools/aarch64-rpi4-linux-gnu/bin/aarch64-rpi4-linux-gnu-gcc
    export CPP=/home/dev/oosman/.leila/toolchains/raspberrypi4/x-tools/aarch64-rpi4-linux-gnu/bin/aarch64-rpi4-linux-gnu-g++
    export AR=/home/dev/oosman/.leila/toolchains/raspberrypi4/x-tools/aarch64-rpi4-linux-gnu/bin/aarch64-rpi4-linux-gnu-ar
    export LDFLAGS=-L/home/dev/oosman/.leila/toolchains/raspberrypi4/x-tools/aarch64-rpi4-linux-gnu/aarch64-rpi4-linux-gnu/sysroot/usr/lib
    ./configure \
    --host=aarch64-rpi4-linux-gnu \
    LDFLAGS=-L/home/dev/oosman/.leila/toolchains/raspberrypi4/x-tools/aarch64-rpi4-linux-gnu/aarch64-rpi4-linux-gnu/sysroot/usr/lib \
    --without-tinfo \
    --without-ncurses \
    --disable-ipv6 \
    --disable-pylibmount \
    --enable-static-programs=fdisk,sfdisk,whereis \
    --prefix=/opt/util-linux/arm \
    --bindir=/opt/util-linux/arm/bin \
    --sbindir=/opt/util-linux/arm/sbin \
    PKG_CONFIG_PATH=/usr/bin/pkg-config \
    PKG_CONFIG_LIBDIR=/home/dev/oosman/.leila/toolchains/raspberrypi4/x-tools/aarch64-rpi4-linux-gnu/aarch64-rpi4-linux-gnu/sysroot/usr/lib \
    AR=/home/dev/oosman/.leila/toolchains/raspberrypi4/x-tools/aarch64-rpi4-linux-gnu/bin/aarch64-rpi4-linux-gnu-ar \
    RANLIB=/home/dev/oosman/.leila/toolchains/raspberrypi4/x-tools/aarch64-rpi4-linux-gnu/bin/aarch64-rpi4-linux-gnu--ranlib \
    CC_FOR_BUILD=/usr/bin/gcc \

    ./configure \
            --host=aarch64-rpi4-linux-gnu \
	    --prefix=`pwd`/out \
	    --enable-shared \
	    --disable-seccomp \
	    CC=/home/dev/oosman/.leila/toolchains/raspberrypi4/x-tools/aarch64-rpi4-linux-gnu/bin/aarch64-rpi4-linux-gnu-gcc \
	    CFLAGS= \
	    CPP=/home/dev/oosman/.leila/toolchains/raspberrypi4/x-tools/aarch64-rpi4-linux-gnu/bin/aarch64-rpi4-linux-gnu-g++ \
	    CPPFLAGS=-I/home/dev/oosman/.leila/toolchains/raspberrypi4/x-tools/aarch64-rpi4-linux-gnu/aarch64-rpi4-linux-gnu/sysroot/usr/include \
	    LDFLAGS=-L/home/dev/oosman/.leila/toolchains/raspberrypi4/x-tools/aarch64-rpi4-linux-gnu/aarch64-rpi4-linux-gnu/sysroot/usr/lib \
	    AR=/home/dev/oosman/.leila/toolchains/raspberrypi4/x-tools/aarch64-rpi4-linux-gnu/bin/aarch64-rpi4-linux-gnu-ar \
	    RANLIB=/home/dev/oosman/.leila/toolchains/raspberrypi4/x-tools/aarch64-rpi4-linux-gnu/bin/aarch64-rpi4-linux-gnu--ranlib \
	    CC_FOR_BUILD=/usr/bin/gcc \
	    CFLAGS_FOR_BUILD= \
	    PKG_CONFIG_PATH=/usr/bin/pkg-config \
	    PKG_CONFIG_LIBDIR=/home/dev/oosman/.leila/toolchains/raspberrypi4/x-tools/aarch64-rpi4-linux-gnu/aarch64-rpi4-linux-gnu/sysroot/usr/lib
}

function buildsystemd()
{
    parseArgs "$@"
    pushd systemd
            git checkout raspi0
	    #meson --reconfigure ./build
	    #meson --cross-file CROSS_FILE  -Dincludedir=../libcap/libcap/include  build
#--includedir /home/dev/oosman/repos/nugen/libcap/libcap/include
#ninja -C build clean #where build is the build directry
rm -fr build
meson setup -Dxz=false -Dopenssl=false \
--prefix /home/dev/oosman/.leila/toolchains/raspberrypi4/x-tools/aarch64-rpi4-linux-gnu/aarch64-rpi4-linux-gnu/sysroot/usr \
--libdir /home/dev/oosman/.leila/toolchains/raspberrypi4/x-tools/aarch64-rpi4-linux-gnu/aarch64-rpi4-linux-gnu/sysroot/usr/lib \
--cross-file CROSS_FILE \
build
            make -j$(getconf _NPROCESSORS_ONLN) VERBOSE=1
    popd
}

function buildlibedgetpu()
{
    parseArgs "$@"

    pushd libedgetpu
        if [ "$arch" == "host" ]; then
                git checkout x86_64
		elif [ "$arch" == "rpi0" ]; then
                git checkout raspi0
		elif [ "$arch" == "rpi4" ]; then
		git checkout raspi4
		elif [ "$arch" == "a53" ]; then
                git checkout a53a
        fi

        if [ ! -d tensorflow ]; then ln -s ../tensorflow tensorflow; fi
        local BAZEL_CACHE_DIR=${HOME}/${DOCKERUSER}/.leila/cache/.bazel
        mkdir -p $BAZEL_CACHE_DIR
        if [ "$clean" == "true" ]; then rm -fr $BAZEL_CACHE_DIR/*;fi

        #git checkout -f coral_crosstool1/cc_toolchain_config.bzl
        #sed -i "s/\$DOCKERUSER/$DOCKERUSER/gi" coral_crosstool1/cc_toolchain_config.bzl
        TEST_TMPDIR=$BAZEL_CACHE_DIR TOOLCHAIN_ROOT_DIR=${TOOLCHAIN_ROOT_DIR} make

        rm -f bazel-build
        bazelbuild=$(TEST_TMPDIR=$BAZEL_CACHE_DIR bazel info --experimental_repo_remote_exec 2>/dev/null | grep "execution_root:" | cut -d " " -f 2 | xargs dirname | xargs dirname)
        ln -sf $bazelbuild bazel-build
    popd
}

function builduserland()
{
    parseArgs "$@"

    pushd userland
        if [ "$arch" != "host" ]; then
            mkdir -p "build${arch}"
                pushd "build${arch}"
                if [ "$clean" == "true" ]; then rm -fr *;fi
		if [ "$arch" == "rpi0" ]; then
                    BUILD_MMAL=TRUE cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_POSITION_INDEPENDENT_CODE=On -DCMAKE_TOOLCHAIN_FILE=$TOOLCHAIN_ROOT_DIR/Toolchain.cmake ../
		elif [ "$arch" == "rpi4" ] || [ "$arch" == "a53" ]; then
                    BUILD_MMAL=TRUE cmake -DCMAKE_BUILD_TYPE=Release -DARM64=ON -DCMAKE_POSITION_INDEPENDENT_CODE=On -DCMAKE_TOOLCHAIN_FILE=$TOOLCHAIN_ROOT_DIR/Toolchain.cmake ../
                fi
                make -j$(getconf _NPROCESSORS_ONLN) VERBOSE=1
            popd
        fi
    popd
}

function buildmmalpp()
{
    parseArgs "$@"

    pushd MMALPP
        if [ "$arch" = "rpi0" ]; then # || [ "$arch" = "rpi4" ] #todo need to fix -I include paths
            git checkout rgb
            mkdir -p "build${arch}"
                if [ "$clean" == "true" ]; then rm -fr rgb-example;fi
                #cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=$TOOLCHAIN_ROOT_DIR/Toolchain.cmake ../
                $CXX_TOOL \
-I./mmalpp -I../userland \
-I${TOOLCHAIN_ROOT_DIR}/sysroots/aarch64-poky-linux/usr/include/c++/8.2.0 \
-I${TOOLCHAIN_ROOT_DIR}/sysroots/aarch64-poky-linux/usr/include/c++/8.2.0/aarch64-poky-linux \
-I${TOOLCHAIN_ROOT_DIR}/sysroots/aarch64-poky-linux/usr/include \
-L../userland/build/lib \
-lmmal -lmmal_util -lmmal_components -lmmal_core -lmmal_vc_client \
-lEGL -lGLESv2 -lOpenVG -lWFC -lbcm_host -lbrcmEGL -lbrcmGLESv2 -lbrcmOpenVG -lbrcmWFC \
-lbrcmjpeg \
-lcontainers \
-ldebug_sym \
-ldtovl \
-lmmal \
-lmmal_components \
-lmmal_core \
-lmmal_omx \
-lmmal_omxutil \
-lmmal_util \
-lmmal_vc_client \
-lopenmaxil \
-lvchiq_arm \
-lvcos \
-lvcsm \
-std=c++17 rgb-example.cpp -o rgb-example
        fi
    popd
}

function buildProject()
{
    parseArgs "$@"
    echo "$TOOLCHAIN_ROOT_DIR/Toolchain.cmake"

    if [ "$arch" == "host" ]; then
        mkdir -p build
        pushd build
            if [ "$clean" == "true" ]; then rm -fr *;fi
            cmake ..
            make -j$(getconf _NPROCESSORS_ONLN) VERBOSE=1
        popd
    else
        mkdir -p "build${arch}"
            pushd "build${arch}"
            if [ "$clean" == "true" ]; then rm -fr *;fi
            cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=$TOOLCHAIN_ROOT_DIR/Toolchain.cmake ../
            make -j$(getconf _NPROCESSORS_ONLN)
        popd
    fi
}

function deployToPi()
{
    parseArgs "$@"
	
    mkdir -p "./build${arch}/stripped"
    cp -f "./build${arch}/libedgetpu/libedgetpu.so" "./build${arch}/stripped/"
    cp -f "./build${arch}/libusb/libusb.so" "./build${arch}/stripped/"
    cp -f ./userland/build/lib/*.so "./build${arch}/stripped/"
    cp -f "./build${arch}/detect" "./build${arch}/stripped/"
    find ./opencv/build${arch} -name "lib*.so.*" -exec cp -Pf -- "{}" ./build${arch}/stripped/ \;
    chrpath -r '$ORIGIN/.' "./build${arch}/stripped/detect"

    $STRIP_TOOL "./build${arch}/stripped/detect"
    for filename in ./build${arch}/stripped/lib*.so; do
        $STRIP_TOOL ${filename}
    done

    ssh-copy-id pi@${ip}
    rsync -uav ./build${arch}/stripped/* pi@${ip}:~/nugen
    rsync -uav download-models.sh pi@${ip}:~/nugen
}

function showResult()
{
    parseArgs "$@"
    if [ "$arch" == "host" ]; then
        file ./build/detect | cowsay
    else
        file "./build${arch}/detect" | cowsay
    fi
}

function main()
{
    parseArgs "$@"
    setupToolchainVars arch="$arch"

    if [ "${deploy}" == "true" ] && [ "${ip}" != "" ]; then
        deployToPi ip="${ip}" STRIP_TOOL="${STRIP_TOOL}"
        return 0
    fi

    if [ "$arch" == "" ]; then
        arch="host"
    fi
    
    cloneRepos "$@"
    buildlibusb "$@"
    buildlibcurl "$@"
    buildopencv "$@"
    #cannot build systemd, for now use libudev.so checked into libedgetpu repo, later remove it when this is building:
    #if [ "$arch" == "rpi0" ] || [ "$arch" = "rpi4" ]; then
    #	buildsystemd "$@"
    #fi
    builduserland "$@"
    buildmmalpp "$@" CXX_TOOL=${CXX_TOOL}
    buildlibedgetpu "$@"
    buildProject "$@"
    showResult "$@"
}

main "$@"

# ./build.sh arch=host clean=true|false #false is default
# ./build.sh arch=rpi0 clean=true|false #false is default
# ./build.sh arch=rpi4 clean=true|false #false is default
# ./build.sh arch=rpi0 deploy=true ip=192.168.1.26
