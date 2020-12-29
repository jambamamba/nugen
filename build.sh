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
                    make -j$(getconf _NPROCESSORS_ONLN)
            popd
        elif [ "$arch" == "rpi" ]; then
            mkdir -p buildpi
            pushd buildpi
                    if [ "$clean" == "true" ]; then rm -fr *;fi
                    cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=$PI_TOOLCHAIN_ROOT_DIR/Toolchain-RaspberryPi.cmake ../
                    make -j$(getconf _NPROCESSORS_ONLN)
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
                    make -j$(getconf _NPROCESSORS_ONLN)
            popd
        elif [ "$arch" == "rpi" ]; then
            mkdir -p buildpi
            pushd buildpi
                    if [ "$clean" == "true" ]; then rm -fr *;fi
                    #cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=$PI_TOOLCHAIN_ROOT_DIR/Toolchain-RaspberryPi.cmake ../
                    #make -j$(getconf _NPROCESSORS_ONLN)
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
                    make -j$(getconf _NPROCESSORS_ONLN)
            popd
        elif [ "$arch" == "rpi" ]; then
            mkdir -p buildpi
            pushd buildpi
                    if [ "$clean" == "true" ]; then rm -fr *;fi
                    cmake -DCMAKE_BUILD_TYPE=Release -DOPENCV_FORCE_3RDPARTY_BUILD=ON -DCMAKE_TOOLCHAIN_FILE=$PI_TOOLCHAIN_ROOT_DIR/Toolchain-RaspberryPi.cmake ../
                    make -j$(getconf _NPROCESSORS_ONLN)
            popd
        fi
    popd
}

function buildsystemd()
{
    parseArgs "$@"
    pushd systemd
            git checkout raspi0
            /home/dev/.local/bin/meson --cross-file CROSS_FILE build
            make -j$(getconf _NPROCESSORS_ONLN)
    popd
}

function buildlibedgetpu()
{
    parseArgs "$@"

    pushd libedgetpu
        if [ "$arch" == "host" ]; then
                git checkout x86_64
        elif [ "$arch" == "rpi" ]; then
                git checkout raspi0
        fi

        if [ ! -d tensorflow ]; then ln -s ../tensorflow tensorflow; fi
        local BAZEL_CACHE_DIR=${HOME}/${DOCKERUSER}/.leila/cache/.bazel
        mkdir -p $BAZEL_CACHE_DIR
        if [ "$clean" == "true" ]; then rm -fr $BAZEL_CACHE_DIR/*;fi

        #git checkout -f coral_crosstool1/cc_toolchain_config.bzl
        #sed -i "s/\$DOCKERUSER/$DOCKERUSER/gi" coral_crosstool1/cc_toolchain_config.bzl
        TEST_TMPDIR=$BAZEL_CACHE_DIR PI_TOOLCHAIN_ROOT_DIR=${PI_TOOLCHAIN_ROOT_DIR} make

        rm -f bazel-build
        bazelbuild=$(TEST_TMPDIR=$BAZEL_CACHE_DIR bazel info --experimental_repo_remote_exec 2>/dev/null | grep "execution_root:" | cut -d " " -f 2 | xargs dirname | xargs dirname)
        ln -sf $bazelbuild bazel-build
    popd
}

function buildProject()
{
    parseArgs "$@"

    if [ "$arch" == "host" ]; then
        mkdir -p build
        pushd build
            if [ "$clean" == "true" ]; then rm -fr *;fi
            cmake ..
            make -j$(getconf _NPROCESSORS_ONLN)
        popd
    elif [ "$arch" == "rpi" ]; then
        mkdir -p buildpi
            pushd buildpi
            if [ "$clean" == "true" ]; then rm -fr *;fi
            cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=$PI_TOOLCHAIN_ROOT_DIR/Toolchain-RaspberryPi.cmake ../
            make -j$(getconf _NPROCESSORS_ONLN)
        popd
    else
        echo "invalid arch supplied, should be  arch=host or arch=rpi"
        exit -1
    fi
}

function deployToPi()
{
    parseArgs "$@"
	
    mkdir -p ./buildpi/stripped
    cp -f ./buildpi/libedgetpu/libedgetpu.so ./buildpi/stripped/
    cp -f ./buildpi/libusb/libusb.so ./buildpi/stripped/
    cp -f ./buildpi/minimal ./buildpi/stripped/
    cp -f ./buildpi/classify ./buildpi/stripped/
    cp -f ./buildpi/detect ./buildpi/stripped/
    find ./opencv/buildpi -name "lib*.so.*" -exec cp -Pf -- "{}" ./buildpi/stripped/ \;
    chrpath -r '$ORIGIN/.' ./buildpi/stripped/minimal
    chrpath -r '$ORIGIN/.' ./buildpi/stripped/classify
    chrpath -r '$ORIGIN/.' ./buildpi/stripped/detect

    $PI_TOOLCHAIN_ROOT_DIR/x-tools/arm-rpi-linux-gnueabihf/bin/arm-rpi-linux-gnueabihf-strip ./buildpi/stripped/minimal
    $PI_TOOLCHAIN_ROOT_DIR/x-tools/arm-rpi-linux-gnueabihf/bin/arm-rpi-linux-gnueabihf-strip ./buildpi/stripped/classify
    $PI_TOOLCHAIN_ROOT_DIR/x-tools/arm-rpi-linux-gnueabihf/bin/arm-rpi-linux-gnueabihf-strip ./buildpi/stripped/detect
    for filename in ./buildpi/stripped/lib*.so; do
        $PI_TOOLCHAIN_ROOT_DIR/x-tools/arm-rpi-linux-gnueabihf/bin/arm-rpi-linux-gnueabihf-strip ${filename}
    done

    ssh-copy-id pi@${ip}
    rsync -uav ./buildpi/stripped/* pi@${ip}:/tmp
    rsync -uav download-models.sh pi@${ip}:/tmp
    rsync -uav models/cat.rgb pi@${ip}:/tmp
    #scp /tmp/mobilenet_v2_1.0_224_quant_edgetpu.tflite pi@${IP}:/tmp/
}

function showResult()
{
    parseArgs "$@"
    if [ "$arch" == "rpi" ]; then
            file ./buildpi/minimal | cowsay
    else
            file ./build/minimal | cowsay
    fi
}

function main()
{
    parseArgs "$@"

    PI_TOOLCHAIN_ROOT_DIR=${HOME}/${DOCKERUSER}/.leila/toolchains/rpi

    if [ "${deploy}" == "true" ] && [ "${ip}" != "" ]; then
        deployToPi ip="${ip}" PI_TOOLCHAIN_ROOT_DIR=${PI_TOOLCHAIN_ROOT_DIR}
        return 0
    fi

    if [ "$arch" == "" ]; then
        arch="host"
#        while true; do
#            read -p "Do you wish to build for host \n(next time try ./build.sh arch=host or ./build.sh arch=rpi)? " yn
#            case $yn in
#                [Yy]* ) arch="host"; break;;
#                [Nn]* ) arch="rpi"; break;;
#                * ) echo "Please answer yes or no.";;
#            esac
#        done
    fi

    cloneRepos clean=$clean
    buildlibusb arch=$arch PI_TOOLCHAIN_ROOT_DIR=${PI_TOOLCHAIN_ROOT_DIR} clean=$clean
    buildlibcurl arch=$arch PI_TOOLCHAIN_ROOT_DIR=${PI_TOOLCHAIN_ROOT_DIR} clean=$clean
    buildopencv arch=$arch PI_TOOLCHAIN_ROOT_DIR=${PI_TOOLCHAIN_ROOT_DIR} clean=$clean
    #cannot build systemd, for now use libudev.so checked into libedgetpu repo, later remove it when this is building:
    #if [ "$arch" == "rpi" ]; then
    #	buildsystemd arch=$arch PI_TOOLCHAIN_ROOT_DIR=${PI_TOOLCHAIN_ROOT_DIR} clean=$clean
    #fi
    buildlibedgetpu arch=$arch PI_TOOLCHAIN_ROOT_DIR=${PI_TOOLCHAIN_ROOT_DIR} clean=$clean
    buildProject arch=$arch PI_TOOLCHAIN_ROOT_DIR=${PI_TOOLCHAIN_ROOT_DIR} clean=$clean
    showResult arch=$arch
}

main "$@"

# ./build.sh arch=host clean=true|false #false is default
# ./build.sh arch=rpi clean=true|false #false is default
# ./build.sh deploy=true ip=192.168.1.26
