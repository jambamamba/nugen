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
struct ModelMetaData
{
    std::string server_uri_;
    std::string model_file_;
    std::string label_uri_;
    std::string label_file_;
    int image_width_;
    int image_height_;

    ModelMetaData(const std::string &server_uri,
                  const std::string &model_file,
                  const std::string &label_uri,
                  const std::string &label_file,
                  int image_width,
                  int image_height)
        : server_uri_(server_uri)
        , model_file_(model_file)
        , label_uri_(label_uri)
        , label_file_(label_file)
        , image_width_(image_width)
        , image_height_(image_height)
    {}
};

ModelMetaData meta_data_[TfLiteInterpreter::Type::NumTypes] = {
    {
        "https://github.com/google-coral/edgetpu/raw/master/test_data/",
        "mobilenet_v2_1.0_224_quant",
        "https://storage.googleapis.com/download.tensorflow.org/data/",
        "ImageNetLabels.txt",
        224,224
    },
    {
        "https://github.com/google-coral/edgetpu/raw/master/test_data/",
        "ssd_mobilenet_v2_coco_quant_postprocess",
        "https://raw.githubusercontent.com/google-coral/test_data/master/",
        "coco_labels.txt",
        300,300
    }
};

struct ModelDownloader
{
    ModelDownloader()
    {
        system("/bin/bash -c \"./download-models.sh\"");
    }
}_;

void RegExParseLabelFromLine(std::unordered_map<int, std::string> &labels, const std::string &line)
{
    std::regex rgx("(\\d*)\\s*([0-9a-zA-Z ,]*)");

    std::smatch matches;
    std::regex_search(line, matches, rgx);

    if(matches.size() > 2)
    {
        int label_id = std::atoi(matches[1].str().c_str());
        std::string label_name = matches[2].str();
        labels[label_id] = label_name;
    }
}

std::unordered_map<int, std::string> LoadLabels(TfLiteInterpreter::Type type)
{
  std::string file_path = "models/" + meta_data_[type].label_file_;

  std::ifstream file(file_path.c_str());
  if(!file)
  {
      std::cerr << "Cannot open " << file_path << "\n";
      exit(-1);
  }

  std::unordered_map<int, std::string> labels;
  int label_id = 0;
  std::string line;
  while (std::getline(file, line))
  {
      absl::RemoveExtraAsciiWhitespace(&line);
      switch(type)
      {
      case TfLiteInterpreter::Type::Classifier:
          labels[label_id++] = line;
          break;
      case TfLiteInterpreter::Type::Detector:
          RegExParseLabelFromLine(labels, line);
          break;
      default:
          exit(-1);
      }
  }

  return labels;
}
}//namespace

TfLiteInterpreter::TfLiteInterpreter(Type type)
    : type_(type)
    , image_dimensions_(meta_data_[type].image_width_, meta_data_[type].image_height_)
    , labels_(LoadLabels(type))
{}

TfLiteInterpreter::~TfLiteInterpreter()
{}

bool TfLiteInterpreter::Create()
{
    const auto& available_tpus = edgetpu::EdgeTpuManager::GetSingleton()->EnumerateEdgeTpu();
    std::cout << "available_tpus.size: " << available_tpus.size() << "\n";

    interpreter_builder_ = (available_tpus.size() > 0) ?
                (std::unique_ptr<InterpreterBuilderInterface>)
                std::make_unique<EdgeTpuInterpreterBuilder>("models/" + meta_data_[type_].model_file_ + "_edgetpu.tflite"):
                (std::unique_ptr<InterpreterBuilderInterface>)
                std::make_unique<TfLiteInterpreterBuilder>("models/" + meta_data_[type_].model_file_ + ".tflite");

    interpreter_ = interpreter_builder_->BuildInterpreter();
    if(!interpreter_)
    {
        std::cerr << "Failed to create interpreter";
        return false;
    }
    return true;
}

TfLiteInterpreter::ImageDimensions TfLiteInterpreter::GetImageDimensions() const
{
    return image_dimensions_;
}

bool TfLiteInterpreter::LoadImage(const std::string &rgb_file) const
{
    auto input = interpreter_->input_tensor(0);

    std::ifstream file(rgb_file, std::ios::binary);
    if(!file)
    {
        std::cerr << "Could not load image for classification: " << rgb_file << "\n";
        std::cerr << "It must be a RGB file, 8 bits per pixel, with dimensions "
                  << meta_data_[type_].image_width_
                  << "x"
                  << meta_data_[type_].image_height_
                  << "\n";
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

bool TfLiteInterpreter::LoadImage(const uint8_t *data, size_t sz) const
{
    auto input = interpreter_->input_tensor(0);
    if(sz != input->bytes)
    {
        std::cerr << "Input file size is " << sz << " bytes, expecting size " << input->bytes << "\n";
        exit(-1);
        return false;
    }
    memcpy(input->data.data, data, input->bytes);

    return true;
}

TfLiteInterpreter::Result TfLiteInterpreter::Inference() const
{
    auto start = std::chrono::steady_clock::now();
    if(interpreter_->Invoke() != kTfLiteOk)
    {
        std::cerr << "Failed to invoke model." << std::endl;
        exit(-1);
    }
    auto end = std::chrono::steady_clock::now();
    if(type_ == TfLiteInterpreter::Type::Classifier)
    {
        auto results = coral::GetClassificationResults(*interpreter_, 0.0f, /*top_k=*/1);
        if(results.size() < 1)
        {
            std::cerr << "Failed to classify image." << std::endl;
            exit(-1);
        }
        std::vector<DetectedObject> objects = { {labels_.at(results[0].id), results[0].score} };
        return Result(std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count(), objects);
    }
    else if(type_ == TfLiteInterpreter::Type::Detector)
    {
        auto results = coral::GetDetectionResults(*interpreter_, 0.0f, /*top_k=*/10);
        std::vector<DetectedObject> objects;
        for(const auto &result : results)
        {
            objects.push_back(DetectedObject(labels_.at(result.id), result.score,
                          cv::Rect(
                              cv::Point(result.bbox.xmin * meta_data_[type_].image_width_,
                                 result.bbox.ymin * meta_data_[type_].image_height_),
                              cv::Point(result.bbox.xmax * meta_data_[type_].image_width_,
                                 result.bbox.ymax * meta_data_[type_].image_height_)
                          )
                        )
                    );
        }
        return Result(std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count(),
                    objects
                );
    }
    else
    {
        exit(-1);
    }
    return Result();
}
