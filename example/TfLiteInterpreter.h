#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include "adapter.h"

namespace tflite {
class Interpreter;
}
class InterpreterBuilderInterface;
class TfLiteInterpreter
{
public:
    struct Result
    {
        int milliseconds_;
        std::string class_;

        Result(int milliseconds,
               const std::string &cls)
            : milliseconds_(milliseconds)
            , class_(cls)
        {}
    };

    TfLiteInterpreter();
    ~TfLiteInterpreter();

    bool Create();
    bool LoadImage(const std::string &rgb_file) const;
    Result Invoke() const;

protected:
    std::unordered_map<int, std::string> labels_;
    std::unique_ptr<InterpreterBuilderInterface> interpreter_builder_;
    std::unique_ptr<tflite::Interpreter> interpreter_;
};
