#pragma once

#include "interpreterBuilderInterface.h"

class TfLiteInterpreterBuilder : public InterpreterBuilderInterface
{
public:
    TfLiteInterpreterBuilder(const std::string &model_path);
    virtual std::unique_ptr<tflite::Interpreter> BuildInterpreter() override;
};
