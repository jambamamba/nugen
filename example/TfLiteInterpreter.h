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
    struct DetectedObject
    {
        std::string class_;
        cv::Rect bounding_rect_;
        float score_;

        DetectedObject(const std::string &cls = "",
                       float score = 0,
                       const cv::Rect &bounding_rect = cv::Rect())
            : class_(cls)
            , score_(score)
            , bounding_rect_(bounding_rect)
        {}
    };

    struct ImageDimensions
    {
        size_t width_;
        size_t height_;
        ImageDimensions(size_t width, size_t height)
            : width_(width)
            , height_(height)
        {}
    };

    struct Result
    {
        int milliseconds_;
        std::vector<DetectedObject> objects_;

        Result(int milliseconds = -1,
               const std::vector<DetectedObject> &objects = std::vector<DetectedObject>())
            : milliseconds_(milliseconds)
            , objects_(objects)
        {}
    };

    TfLiteInterpreter(Type type);
    ~TfLiteInterpreter();

    bool Create();
    ImageDimensions GetImageDimensions() const;
    bool LoadImage(const std::string &rgb_file) const;
    bool LoadImage(const uint8_t *data, size_t sz) const;
    Result Inference() const;

protected:
    Type type_;
    ImageDimensions image_dimensions_;
    std::unordered_map<int, std::string> labels_;
    std::unique_ptr<InterpreterBuilderInterface> interpreter_builder_;
    std::unique_ptr<tflite::Interpreter> interpreter_;
};
