#!/bin/bash -xe
set -xe

export CC=/usr/bin/gcc
export CXX=/usr/bin/g++

PI_TOOLCHAIN_ROOT_DIR=${HOME}/${DOCKERUSER}/pi

if [ ! -d libusb ]; then
   git clone https://github.com/jambamamba/libusb.git
fi
if [ ! -d systemd ]; then
   git clone https://github.com/systemd/systemd.git
fi
if [ ! -d libedgetpu ]; then
   git clone https://github.com/jambamamba/libedgetpu.git
fi
if [ ! -d tensorflow ]; then
   git clone https://github.com/jambamamba/tensorflow.git
fi

pushd libusb
	git checkout x86_64
	if [ ! -f config.h ]; then 
		./autogen.sh
	fi
	mkdir -p build
	pushd build
		cmake ..
		make -j$(getconf _NPROCESSORS_ONLN)
	popd
	
	if [ -d $PI_TOOLCHAIN_ROOT_DIR ]; then
	mkdir -p buildpi
	pushd buildpi
		cmake -DCMAKE_TOOLCHAIN_FILE=$PI_TOOLCHAIN_ROOT_DIR/Toolchain-RaspberryPi.cmake ../
		make -j$(getconf _NPROCESSORS_ONLN)
	popd
	fi
	
popd

pushd libedgetpu
	git checkout x86_64
	if [ ! -d tensorflow ]; then ln -s ../tensorflow tensorflow; fi
	mkdir -p /tmp/.bazel
	TEST_TMPDIR=/tmp/.bazel make
	rm -f bazel-build
	bazelbuild=$(TEST_TMPDIR=/tmp/.bazel bazel info --experimental_repo_remote_exec 2>/dev/null | grep "execution_root:" | cut -d " " -f 2 | xargs dirname | xargs dirname)
	ln -s $bazelbuild bazel-build
popd

mkdir -p build
	pushd build
	cmake ..
	make -j$(getconf _NPROCESSORS_ONLN)
popd

if [ -d $PI_TOOLCHAIN_ROOT_DIR ]; then
mkdir -p buildpi
	pushd buildpi
	cmake -DCMAKE_TOOLCHAIN_FILE=$PI_TOOLCHAIN_ROOT_DIR/Toolchain-RaspberryPi.cmake ../
	make -j$(getconf _NPROCESSORS_ONLN)
popd
fi

if [ ! -f /tmp/mobilenet_v1_1.0_224_quant_edgetpu.tflite ]; then
   pushd /tmp
   wget https://github.com/google-coral/edgetpu/raw/master/test_data/mobilenet_v1_1.0_224_quant_edgetpu.tflite
   popd
fi

if [ -d $PI_TOOLCHAIN_ROOT_DIR ]; then
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
	
	file ./buildpi/minimal | cowsay
else
	file ./build/minimal | cowsay
fi

