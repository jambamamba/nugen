#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <opencv2/core/types.hpp>

#include "adapter.h"

namespace tflite {
class Interpreter;
}
class InterpreterBuilderInterface;
class TfLiteInterpreter
{
public:
    enum Type
    {
        Classifier,
        Detector,
        NumTypes
    };
    struct Result
    {
        int milliseconds_;
        std::string class_;
        cv::Rect bounding_rect_;

        Result(int milliseconds = -1,
               const std::string &cls = "",
               const cv::Rect &bounding_rect = cv::Rect())
            : milliseconds_(milliseconds)
            , class_(cls)
            , bounding_rect_(bounding_rect)
        {}
    };

    TfLiteInterpreter(Type type);
    ~TfLiteInterpreter();

    bool Create();
    bool LoadImage(const std::string &rgb_file) const;
    Result Inference() const;

protected:
    Type type_;
    std::unordered_map<int, std::string> labels_;
    std::unique_ptr<InterpreterBuilderInterface> interpreter_builder_;
    std::unique_ptr<tflite::Interpreter> interpreter_;
};
