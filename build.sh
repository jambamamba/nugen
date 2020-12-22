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
                                cmake -DCMAKE_TOOLCHAIN_FILE=$PI_TOOLCHAIN_ROOT_DIR/Toolchain-RaspberryPi.cmake ../
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
			cmake -DCMAKE_TOOLCHAIN_FILE=$PI_TOOLCHAIN_ROOT_DIR/Toolchain-RaspberryPi.cmake ../
			make -j$(getconf _NPROCESSORS_ONLN)
		popd
	else
		echo "invalid arch supplied, should be  arch=host or arch=rpi"
		exit -1
	fi
}

function downloadTfModel()
{
	if [ ! -f /tmp/mobilenet_v1_1.0_224_quant_edgetpu.tflite ]; then
	   pushd /tmp
	   wget https://github.com/google-coral/edgetpu/raw/master/test_data/mobilenet_v1_1.0_224_quant_edgetpu.tflite
	   popd
	fi
}

function stripCrossCompiledBinaries()
{
    parseArgs "$@"
	
    mkdir -p ./buildpi/stripped
    cp -f ./buildpi/libedgetpu/libedgetpu.so ./buildpi/stripped/
    cp -f ./buildpi/libusb/libusb.so ./buildpi/stripped/
    cp -f ./buildpi/minimal ./buildpi/stripped/
    chrpath -r '$ORIGIN/.' ./buildpi/stripped/minimal

    $PI_TOOLCHAIN_ROOT_DIR/x-tools/arm-rpi-linux-gnueabihf/bin/arm-rpi-linux-gnueabihf-strip ./buildpi/stripped/minimal
    $PI_TOOLCHAIN_ROOT_DIR/x-tools/arm-rpi-linux-gnueabihf/bin/arm-rpi-linux-gnueabihf-strip ./buildpi/stripped/libedgetpu.so
    $PI_TOOLCHAIN_ROOT_DIR/x-tools/arm-rpi-linux-gnueabihf/bin/arm-rpi-linux-gnueabihf-strip ./buildpi/stripped/libusb.so

    #scp ./buildpi/stripped/* pi@192.168.1.26:/tmp
    #scp /tmp/mobilenet_v1_1.0_224_quant_edgetpu.tflite pi@192.168.1.26:/tmp/
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
	
	if [ "$arch" == "" ]; then
            arch="host"
#            while true; do
#                read -p "Do you wish to build for host \n(next time try ./build.sh arch=host or ./build.sh arch=rpi)? " yn
#                case $yn in
#                    [Yy]* ) arch="host"; break;;
#                    [Nn]* ) arch="rpi"; break;;
#                    * ) echo "Please answer yes or no.";;
#                esac
#            done
	fi

	PI_TOOLCHAIN_ROOT_DIR=${HOME}/${DOCKERUSER}/.leila/toolchains/rpi
	cloneRepos clean=$clean
	buildlibusb arch=$arch PI_TOOLCHAIN_ROOT_DIR=${PI_TOOLCHAIN_ROOT_DIR} clean=$clean
	#cannot build systemd, for now use libudev.so checked into libedgetpu repo, later remove it when this is building:
	#if [ "$arch" == "rpi" ]; then
	#	buildsystemd arch=$arch PI_TOOLCHAIN_ROOT_DIR=${PI_TOOLCHAIN_ROOT_DIR} clean=$clean
	#fi
	buildlibedgetpu arch=$arch PI_TOOLCHAIN_ROOT_DIR=${PI_TOOLCHAIN_ROOT_DIR} clean=$clean
	buildProject arch=$arch PI_TOOLCHAIN_ROOT_DIR=${PI_TOOLCHAIN_ROOT_DIR} clean=$clean
	downloadTfModel
	if [ "$arch" == "rpi" ]; then
		stripCrossCompiledBinaries
	fi
	showResult arch=$arch 
}

main "$@"

# ./build.sh arch=host clean=true|false #false is default
# ./build.sh arch=rpi clean=true|false #false is default

