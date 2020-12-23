#include "TfLiteInterpreterBuilder.h"

#include <iostream>

#include "tensorflow/lite/builtin_op_data.h"
#include "tensorflow/lite/interpreter.h"
#include "tensorflow/lite/kernels/register.h"
#include "tensorflow/lite/model.h"
#include "tensorflow/lite/optional_debug_tools.h"

TfLiteInterpreterBuilder::TfLiteInterpreterBuilder(const std::string &model_path)
    : InterpreterBuilderInterface(model_path)
{}

std::unique_ptr<tflite::Interpreter> TfLiteInterpreterBuilder::BuildInterpreter()
{
    model_ = tflite::FlatBufferModel::BuildFromFile(model_path_.c_str());

    tflite::ops::builtin::BuiltinOpResolver resolver;
    std::unique_ptr<tflite::Interpreter> interpreter;
    if (tflite::InterpreterBuilder(*model_.get(), resolver)(&interpreter) != kTfLiteOk) {
      std::cerr << "Failed to build interpreter." << std::endl;
    }
    // Bind given context with interpreter.
    interpreter->SetNumThreads(1);
    if (interpreter->AllocateTensors() != kTfLiteOk) {
      std::cerr << "Failed to allocate tensors." << std::endl;
    }

    return interpreter;
}
