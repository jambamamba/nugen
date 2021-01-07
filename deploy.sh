#!/bin/bash -xe
set -xe

pushd buildpi 
make -j8 
popd
./build.sh deploy=true ip=192.168.1.26
