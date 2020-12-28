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
}

function main()
{
    mkdir -p models

    pushd models
        #use for classification
        local modal_name="mobilenet_v2_1.0_224_quant"
        local server_url="https://github.com/google-coral/edgetpu/raw/master/test_data"
        downloadFile server_url="${server_url}" model_file="${modal_name}.tflite"
        downloadFile server_url="${server_url}" model_file="${modal_name}_edgetpu.tflite"
        downloadFile server_url="https://storage.googleapis.com/download.tensorflow.org/data" model_file="ImageNetLabels.txt"

        #use for object detection
        local modal_name="ssd_mobilenet_v2_coco_quant_postprocess"
        local server_url="https://github.com/google-coral/edgetpu/raw/master/test_data"
        downloadFile server_url="${server_url}" model_file="${modal_name}.tflite"
        downloadFile server_url="${server_url}" model_file="${modal_name}_edgetpu.tflite"
        downloadFile server_url="https://raw.githubusercontent.com/google-coral/test_data/master" model_file="coco_labels.txt"
    popd
}
main "$@"
