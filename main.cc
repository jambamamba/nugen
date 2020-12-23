#include <iostream>
#include <memory>

#include "edgetpu.h"
#include "tensorflow/lite/builtin_op_data.h"
#include "tensorflow/lite/interpreter.h"
#include "tensorflow/lite/kernels/register.h"
#include "tensorflow/lite/model.h"
#include "tensorflow/lite/optional_debug_tools.h"
#include "tflite/public/edgetpu.h"

namespace  {
std::unique_ptr<tflite::Interpreter> BuildEdgeTpuInterpreter(
    const tflite::FlatBufferModel& model,
    edgetpu::EdgeTpuContext* edgetpu_context)
{
  tflite::ops::builtin::BuiltinOpResolver resolver;
  resolver.AddCustom(edgetpu::kCustomOp, edgetpu::RegisterCustomOp());
  std::unique_ptr<tflite::Interpreter> interpreter;
  if (tflite::InterpreterBuilder(model, resolver)(&interpreter) != kTfLiteOk) {
    std::cerr << "Failed to build interpreter." << std::endl;
  }
  // Bind given context with interpreter.
  interpreter->SetExternalContext(kTfLiteEdgeTpuContext, edgetpu_context);
  interpreter->SetNumThreads(1);
  if (interpreter->AllocateTensors() != kTfLiteOk) {
    std::cerr << "Failed to allocate tensors." << std::endl;
  }
  return interpreter;
}
std::unique_ptr<tflite::Interpreter> BuildTfLiteInterpreter(
    const tflite::FlatBufferModel& model)
{
  tflite::ops::builtin::BuiltinOpResolver resolver;
  std::unique_ptr<tflite::Interpreter> interpreter;
  if (tflite::InterpreterBuilder(model, resolver)(&interpreter) != kTfLiteOk) {
    std::cerr << "Failed to build interpreter." << std::endl;
  }
  // Bind given context with interpreter.
  interpreter->SetNumThreads(1);
  if (interpreter->AllocateTensors() != kTfLiteOk) {
    std::cerr << "Failed to allocate tensors." << std::endl;
  }
  return interpreter;
}
}//namespace

int main(int argc, char**argv)
{
//    const std::string model_path = "/tmp/mobilenet_v1_1.0_224_quant_edgetpu.tflite";
    const std::string model_path = "/home/dev/oosman/repos/edgetpu-minimal/models/mobilenet_v1_0.25_128_quant.tflite";
    std::unique_ptr<tflite::FlatBufferModel> model =
            tflite::FlatBufferModel::BuildFromFile(model_path.c_str());

//    const auto& available_tpus = edgetpu::EdgeTpuManager::GetSingleton()->EnumerateEdgeTpu();
//    std::cout << "available_tpus.size: " << available_tpus.size() << "\n"; // hopefully we'll see 1 here

//    std::shared_ptr<edgetpu::EdgeTpuContext> edgetpu_context =
//        edgetpu::EdgeTpuManager::GetSingleton()->OpenDevice();

//    std::unique_ptr<tflite::Interpreter> model_interpreter =
//        BuildEdgeTpuInterpreter(*model, edgetpu_context.get());
    std::unique_ptr<tflite::Interpreter> model_interpreter =
            BuildTfLiteInterpreter(*model);

    if(model_interpreter->Invoke() != kTfLiteOk) {
        std::cerr << "Failed to invoke model." << std::endl;
    }
    tflite::PrintInterpreterState(model_interpreter.get());
    std::cout << "Done\n";
    return 0;
}
