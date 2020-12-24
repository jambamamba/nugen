#!/bin/bash -xe
set -xe

function main()
{
       mkdir -p models
       pushd models
           local file="mobilenet_v1_1.0_224_quant_edgetpu.tflite"
           if [ ! -f "${file}" ]; then
              wget "https://github.com/google-coral/edgetpu/raw/master/test_data/${file}"
           fi
           if [ ! -f "/tmp/${file}" ]; then
              cp -f ${file} /tmp
           fi

           local file="mobilenet_v1_1.0_224.tgz"
           if [ ! -f "${file}" ]; then
              wget "https://storage.googleapis.com/download.tensorflow.org/models/mobilenet_v1_2018_02_22/${file}"
           fi
           local mobilenet_dir=""${file%.*}"" #remove extension .tgz
           local tflite_file="${mobilenet_dir}.tflite"
           if [ ! -f "/tmp/${tflite_file}" ]; then
              tar -zxvf ${file} ./${tflite_file}
              cp -f ${tflite_file} /tmp
           fi

           local frozen_file="${mobilenet_dir}_frozen.tgz"
           if [ ! -f "${frozen_file}" ]; then
             wget "https://storage.googleapis.com/download.tensorflow.org/models/${frozen_file}"
           fi
           if [ ! -d "${mobilenet_dir}" ]; then
             tar -zxvf ${frozen_file} "${mobilenet_dir}/labels.txt"
           fi
           if [ ! -f "${mobilenet_dir}_labels.txt" ]; then
           cp -f "${mobilenet_dir}/labels.txt" "/tmp/${mobilenet_dir}_labels.txt"
           fi
       popd
}
main "$@"
