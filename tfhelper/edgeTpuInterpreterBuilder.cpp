#include "edgeTpuInterpreterBuilder.h"

#include <iostream>
#include <nzlogger.h>

#include "edgetpu.h"
#include "tensorflow/lite/builtin_op_data.h"
#include "tensorflow/lite/interpreter.h"
#include "tensorflow/lite/kernels/register.h"
#include "tensorflow/lite/model.h"
#include "tensorflow/lite/optional_debug_tools.h"

NEW_LOG_CATEGORY(EdgeTpu)

EdgeTpuInterpreterBuilder::EdgeTpuInterpreterBuilder(const std::string &model_path)
    : InterpreterBuilderInterface(model_path)
{}

std::unique_ptr<tflite::Interpreter> EdgeTpuInterpreterBuilder::BuildInterpreter()
{
    model_ = tflite::FlatBufferModel::BuildFromFile(model_path_.c_str());

    edgetpu_context_ = edgetpu::EdgeTpuManager::GetSingleton()->OpenDevice();
    if(!edgetpu_context_)
    {
        LOG_C(EdgeTpu, WARNING) << "Failed to open USB device.";
        return std::unique_ptr<tflite::Interpreter>();
    }
    tflite::ops::builtin::BuiltinOpResolver resolver;
    resolver.AddCustom(edgetpu::kCustomOp, edgetpu::RegisterCustomOp());
    std::unique_ptr<tflite::Interpreter> interpreter;
    if (tflite::InterpreterBuilder(*model_.get(), resolver)(&interpreter) != kTfLiteOk)
    {
      LOG_C(EdgeTpu, WARNING) << "Failed to build interpreter.";
    }
    // Bind given context with interpreter.
    interpreter->SetExternalContext(kTfLiteEdgeTpuContext, edgetpu_context_.get());
    interpreter->SetNumThreads(1);
    if (interpreter->AllocateTensors() != kTfLiteOk)
    {
      LOG_C(EdgeTpu, WARNING) << "Failed to allocate tensors.";
    }
    return interpreter;
}
