#!/bin/bash -xe
set -xe

function parseArgs()
{
  for change in $@; do
      set -- `echo $change | tr '=' ' '`
      echo "variable name == $1  and variable value == $2"
      #can assign value to a variable like below
      eval $1=$2;
  done
}

function downloadFile()
{
    parseArgs "$@"

    if [ ! -f "${model_file}" ]; then
       wget "${server_url}/${model_file}"
    fi
    if [ ! -f "/tmp/${model_file}" ]; then
       local pwd=${PWD}
       pushd /tmp
       ln -s "${pwd}/${model_file}" /tmp
       popd
    fi
}

function main()
{
    mkdir -p models
    local server_url="https://github.com/google-coral/edgetpu/raw/master/test_data"
    local modal_name="mobilenet_v2_1.0_224_quant"

    pushd models
        downloadFile server_url="${server_url}" model_file="${modal_name}.tflite"
        downloadFile server_url="${server_url}" model_file="${modal_name}_edgetpu.tflite"
        downloadFile server_url="https://storage.googleapis.com/download.tensorflow.org/data" model_file="ImageNetLabels.txt"
    popd
}
main "$@"
