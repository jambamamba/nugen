#pragma once

#include "interpreterBuilderInterface.h"

namespace edgetpu {
class EdgeTpuContext;
}
class EdgeTpuInterpreterBuilder : public InterpreterBuilderInterface
{
public:
    EdgeTpuInterpreterBuilder(const std::string &model_path);

    virtual std::unique_ptr<tflite::Interpreter> BuildInterpreter() override;

protected:
    std::shared_ptr<edgetpu::EdgeTpuContext> edgetpu_context_;
};
