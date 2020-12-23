#pragma once

#include <memory>

namespace tflite {
    class Interpreter;
    class FlatBufferModel;
}

class InterpreterBuilderInterface
{
public:
    InterpreterBuilderInterface(const std::string &model_path);
    virtual ~InterpreterBuilderInterface();

    virtual std::unique_ptr<tflite::Interpreter> BuildInterpreter() = 0;

protected:
    std::string model_path_;
    std::unique_ptr<tflite::FlatBufferModel> model_;
};
