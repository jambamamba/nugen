#include "TfLiteInterpreter.h"

#include <absl/strings/ascii.h>
#include <absl/strings/numbers.h>
#include <absl/strings/str_split.h>
#include <absl/types/span.h>
#include <chrono>
#include <edgetpu.h>
#include <fstream>
#include <iostream>
#include <memory>
#include <numeric>
#include <regex>
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
  int label_id = 0;
  while (std::getline(file, line))
  {
    absl::RemoveExtraAsciiWhitespace(&line);
    labels[label_id] = line;
    label_id++;
  }

  return labels;
}
void DownloadModels()
{
    system("/bin/bash -c \"./download-models.sh\"");
}
}//namespace

TfLiteInterpreter::TfLiteInterpreter()
    : labels_(LoadLabels("/tmp/ImageNetLabels.txt"))
{
    DownloadModels();
}

TfLiteInterpreter::~TfLiteInterpreter()
{
}

bool TfLiteInterpreter::Create()
{
    const auto& available_tpus = edgetpu::EdgeTpuManager::GetSingleton()->EnumerateEdgeTpu();
    std::cout << "available_tpus.size: " << available_tpus.size() << "\n";

    interpreter_builder_ = (available_tpus.size() > 0) ?
                (std::unique_ptr<InterpreterBuilderInterface>)
                std::make_unique<EdgeTpuInterpreterBuilder>("/tmp/mobilenet_v1_1.0_224_quant_edgetpu.tflite"):
                (std::unique_ptr<InterpreterBuilderInterface>)
                std::make_unique<TfLiteInterpreterBuilder>("/tmp/mobilenet_v2_1.0_224_quant.tflite");

    interpreter_ = interpreter_builder_->BuildInterpreter();
    if(!interpreter_)
    {
        std::cerr << "Failed to create interpreter";
        return false;
    }
    return true;
}

bool TfLiteInterpreter::LoadImage(const std::string &rgb_file) const
{
    auto input = interpreter_->input_tensor(0);

    std::ifstream file(rgb_file, std::ios::binary);
    if(!file)
    {
        std::cerr << "Could not load image for classification: " << rgb_file << "\n";
        std::cerr << "It must be a RGB file, 8 bits per pixel, with dimensions 224 x 224 \n";
        exit(-1);
        return false;
    }
    file.seekg(0, std::ios_base::end);
    if(file.tellg() != static_cast<long>(input->bytes))
    {
        std::cerr << "Input file size is " << file.tellg() << " bytes, expecting size " << input->bytes << "\n";
        exit(-1);
        return false;
    }
    file.seekg(0, std::ios_base::beg);
    file.read((char*)input->data.data, input->bytes);

    return true;
}

TfLiteInterpreter::Result TfLiteInterpreter::Invoke() const
{
    auto start = std::chrono::steady_clock::now();
    if(interpreter_->Invoke() != kTfLiteOk)
    {
        std::cerr << "Failed to invoke model." << std::endl;
        exit(-1);
    }
    auto end = std::chrono::steady_clock::now();
    auto results = coral::GetClassificationResults(*interpreter_, 0.0f, /*top_k=*/3);

    Result ret (std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count(),
                  labels_.at(results[0].id)
                );
//    for (auto result : results)
//    {
//      std::cout << "---------------------------" << std::endl;
//      std::cout << "Score: " << result.score
//                << ", label id: " << result.id
//                << ", " << ret.class_
//                << ", time elapsed: "
//                << ret.milliseconds_
//                << " milliseconds"
//                << "\n";
//    }
    return ret;
}
