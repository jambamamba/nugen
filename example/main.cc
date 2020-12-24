#include <fstream>
#include <iostream>
#include <memory>
#include <numeric>
#include <regex>

#include <absl/strings/ascii.h>
#include <absl/strings/numbers.h>
#include <absl/strings/str_split.h>
#include <absl/types/span.h>
#include <edgetpu.h>
#include <tensorflow/lite/builtin_op_data.h>
#include <tensorflow/lite/interpreter.h>
#include <tensorflow/lite/kernels/register.h>
#include <tensorflow/lite/model.h>
#include <tensorflow/lite/optional_debug_tools.h>
#include <tflite/public/edgetpu.h>

#include "adapter.h"
#include "EdgeTpuInterpreterBuilder.h"
#include "TfLiteInterpreterBuilder.h"

namespace  {
void DownloadModels()
{
    system("/bin/bash -c \"./download-models.sh\"");
}
std::unordered_map<int, std::string> LoadLabels(const std::string& file_path)
{
  std::unordered_map<int, std::string> labels;
  std::ifstream file(file_path.c_str());
  if(!file)
  {
      std::cerr << "Cannot open " << file_path << "\n";
      exit(-1);
  }

  std::string line;
  while (std::getline(file, line))
  {
    absl::RemoveExtraAsciiWhitespace(&line);
    std::regex rgx("(\\d*)\\:([0-9a-zA-Z ,]*)");

    std::smatch matches;
    std::regex_search(line, matches, rgx);

    if(matches.size() > 2)
    {
        int label_id = std::atoi(matches[1].str().c_str());
        std::string label_name = matches[2].str();
        labels[label_id] = label_name;
    }
  }

  return labels;
}
}//namespace
int main(int argc, char**argv)
{
    const auto& available_tpus = edgetpu::EdgeTpuManager::GetSingleton()->EnumerateEdgeTpu();
    std::cout << "available_tpus.size: " << available_tpus.size() << "\n";

    DownloadModels();
    auto labels = LoadLabels("/tmp/mobilenet_v1_1.0_224_labels.txt");

    std::unique_ptr<InterpreterBuilderInterface> interpreter_builder;
    interpreter_builder = (available_tpus.size() > 0) ?
                (std::unique_ptr<InterpreterBuilderInterface>)
                std::make_unique<EdgeTpuInterpreterBuilder>("/tmp/mobilenet_v1_1.0_224_quant_edgetpu.tflite"):
                (std::unique_ptr<InterpreterBuilderInterface>)
                std::make_unique<TfLiteInterpreterBuilder>("/tmp/mobilenet_v1_1.0_224.tflite");

    auto interpreter = interpreter_builder->BuildInterpreter();
    if(!interpreter)
    {
        std::cerr << "Failed to create interpreter";
        return -1;
    }

    auto input = interpreter->input_tensor(0);
    {
        std::string file_path("/home/dev/oosman/repos/edgetpu-minimal/models/cat.rgb");
        std::ifstream file(file_path, std::ios::binary);
        if(!file)
        {
            std::cerr << "Could not load image for classification: " << file_path << "\n";
            std::cerr << "It must be a RGB file, 8 bits per pixel, with dimensions 224 x 224 \n";
            exit(-1);
        }
        file.read((char*)input->data.data, input->bytes);
    }

    if(interpreter->Invoke() != kTfLiteOk)
    {
        std::cerr << "Failed to invoke model." << std::endl;
        return -1;
    }

    coral::GetClassificationResults(*interpreter, 0.0f, /*top_k=*/3);
    for (auto result : coral::GetClassificationResults(*interpreter, 0.0f, /*top_k=*/3))
    {
      std::cout << "---------------------------" << std::endl;
      std::cout << "Score: " << (int)(result.score * 100) << "%, " << labels[result.id] << std::endl;
    }

//    tflite::PrintInterpreterState(interpreter.get());
    std::cout << "Done\n";
    return 0;
}
