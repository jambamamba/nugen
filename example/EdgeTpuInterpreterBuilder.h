#pragma once

#include "InterpreterBuilderInterface.h"

class EdgeTpuInterpreterBuilder : public InterpreterBuilderInterface
{
public:
    EdgeTpuInterpreterBuilder(const std::string &model_path);

    virtual std::unique_ptr<tflite::Interpreter> BuildInterpreter() override;
};
