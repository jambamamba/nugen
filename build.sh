#!/bin/bash -xe
set -xe

export CC=/usr/bin/gcc
export CXX=/usr/bin/g++

if [ ! -d libedgetpu ]; then
   git clone https://github.com/jambamamba/libedgetpu.git
fi
pushd libedgetpu
if [ ! -d tensorflow ]; then
   git clone https://github.com/jambamamba/tensorflow.git
fi
if [ ! -d libusb ]; then
   git clone https://github.com/jambamamba/libusb.git
fi

pushd libusb
git checkout x86_64
./autogen.sh
mkdir -p build
pushd build
cmake ..
make -j$(getconf _NPROCESSORS_ONLN)
popd
popd

git checkout x86_64
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

if [ ! -f /tmp/mobilenet_v1_1.0_224_quant_edgetpu.tflite ]; then
   pushd /tmp
   wget https://github.com/google-coral/edgetpu/raw/master/test_data/mobilenet_v1_1.0_224_quant_edgetpu.tflite
   popd
fi

./build/minimal
