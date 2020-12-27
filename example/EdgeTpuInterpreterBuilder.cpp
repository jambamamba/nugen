#include "EdgeTpuInterpreterBuilder.h"

#include <iostream>

#include "edgetpu.h"
#include "tensorflow/lite/builtin_op_data.h"
#include "tensorflow/lite/interpreter.h"
#include "tensorflow/lite/kernels/register.h"
#include "tensorflow/lite/model.h"
#include "tensorflow/lite/optional_debug_tools.h"

EdgeTpuInterpreterBuilder::EdgeTpuInterpreterBuilder(const std::string &model_path)
    : InterpreterBuilderInterface(model_path)
{}

std::unique_ptr<tflite::Interpreter> EdgeTpuInterpreterBuilder::BuildInterpreter()
{
    model_ = tflite::FlatBufferModel::BuildFromFile(model_path_.c_str());

    edgetpu_context_ = edgetpu::EdgeTpuManager::GetSingleton()->OpenDevice();
    if(!edgetpu_context_)
    {
        std::cerr << "Failed to open USB device." << std::endl;
        return std::unique_ptr<tflite::Interpreter>();
    }
    tflite::ops::builtin::BuiltinOpResolver resolver;
    resolver.AddCustom(edgetpu::kCustomOp, edgetpu::RegisterCustomOp());
    std::unique_ptr<tflite::Interpreter> interpreter;
    if (tflite::InterpreterBuilder(*model_.get(), resolver)(&interpreter) != kTfLiteOk)
    {
      std::cerr << "Failed to build interpreter." << std::endl;
    }
    // Bind given context with interpreter.
    interpreter->SetExternalContext(kTfLiteEdgeTpuContext, edgetpu_context_.get());
    interpreter->SetNumThreads(1);
    if (interpreter->AllocateTensors() != kTfLiteOk)
    {
      std::cerr << "Failed to allocate tensors." << std::endl;
    }
    return interpreter;
}
