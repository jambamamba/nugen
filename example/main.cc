#include <iostream>
#include <memory>

#include "edgetpu.h"
#include "tensorflow/lite/builtin_op_data.h"
#include "tensorflow/lite/interpreter.h"
#include "tensorflow/lite/kernels/register.h"
#include "tensorflow/lite/model.h"
#include "tensorflow/lite/optional_debug_tools.h"
#include "tflite/public/edgetpu.h"

#include "EdgeTpuInterpreterBuilder.h"
#include "TfLiteInterpreterBuilder.h"

int main(int argc, char**argv)
{
    const auto& available_tpus = edgetpu::EdgeTpuManager::GetSingleton()->EnumerateEdgeTpu();
    std::cout << "available_tpus.size: " << available_tpus.size() << "\n";

    std::unique_ptr<InterpreterBuilderInterface> interpreter_builder;
    if(available_tpus.size() > 0)
    {
        interpreter_builder = std::make_unique<EdgeTpuInterpreterBuilder>("/tmp/mobilenet_v1_1.0_224_quant_edgetpu.tflite");
    }
    else
    {
        interpreter_builder = std::make_unique<TfLiteInterpreterBuilder>("/tmp/mobilenet_v1_0.25_128_quant.tflite");
    }
    auto model_interpreter = interpreter_builder->BuildInterpreter();

    if(model_interpreter->Invoke() != kTfLiteOk) {
        std::cerr << "Failed to invoke model." << std::endl;
    }
    tflite::PrintInterpreterState(model_interpreter.get());
    std::cout << "Done\n";
    return 0;
}
