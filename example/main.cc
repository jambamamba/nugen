#include <fstream>
#include <iostream>
#include <memory>
#include <numeric>

#include "absl/types/span.h"
#include "edgetpu.h"
#include "tensorflow/lite/builtin_op_data.h"
#include "tensorflow/lite/interpreter.h"
#include "tensorflow/lite/kernels/register.h"
#include "tensorflow/lite/model.h"
#include "tensorflow/lite/optional_debug_tools.h"
#include "tflite/public/edgetpu.h"
//#include "/home/dev/oosman/repos/libcoral/coral/tflite_utils.h"
#include "adapter.h"

#include "EdgeTpuInterpreterBuilder.h"
#include "TfLiteInterpreterBuilder.h"

//namespace  {
//inline absl::Span<const int> TensorShape(const TfLiteTensor& tensor) {
//  return absl::Span<const int>(tensor.dims->data, tensor.dims->size);
//}
//inline int TensorSize(const TfLiteTensor& tensor) {
//  auto shape = TensorShape(tensor);
//  return std::accumulate(shape.begin(), shape.end(), 1, std::multiplies<int>());
//}
//template <typename T>
//absl::Span<const T> TensorData(const TfLiteTensor& tensor) {
//  return absl::MakeSpan(reinterpret_cast<const T*>(tensor.data.data),
//                        tensor.bytes / sizeof(T));
//}
//template <typename T>
//std::vector<T> DequantizeTensor(const TfLiteTensor& tensor) {
//  const auto scale = tensor.params.scale;
//  const auto zero_point = tensor.params.zero_point;
//  std::vector<T> result(TensorSize(tensor));

//  if (tensor.type == kTfLiteUInt8)
//    Dequantize(TensorData<uint8_t>(tensor), result.begin(), scale, zero_point);
//  else if (tensor.type == kTfLiteInt8)
//    Dequantize(TensorData<int8_t>(tensor), result.begin(), scale, zero_point);
//  else
//    std::cerr << "Unsupported tensor type: " << tensor.type;

//  return result;
//}
//std::vector<Class> GetClassificationResults(
//    const tflite::Interpreter& interpreter, float threshold, size_t top_k) {
//  const auto& tensor = *interpreter.output_tensor(0);
//  if (tensor.type == kTfLiteUInt8 || tensor.type == kTfLiteInt8) {
//        return GetClassificationResults(DequantizeTensor<float>(tensor), threshold,
//                                        top_k);
//  } else if (tensor.type == kTfLiteFloat32) {
//    return GetClassificationResults(TensorData<float>(tensor), threshold,
//                                    top_k);
//  } else {
//    LOG(FATAL) << "Unsupported tensor type: " << tensor.type;
//  }
//}
//}
int main(int argc, char**argv)
{
    const auto& available_tpus = edgetpu::EdgeTpuManager::GetSingleton()->EnumerateEdgeTpu();
    std::cout << "available_tpus.size: " << available_tpus.size() << "\n";

    std::unique_ptr<InterpreterBuilderInterface> interpreter_builder;
    interpreter_builder = (available_tpus.size() > 0) ?
                (std::unique_ptr<InterpreterBuilderInterface>) std::make_unique<EdgeTpuInterpreterBuilder>("/tmp/mobilenet_v1_1.0_224_quant_edgetpu.tflite"):
                (std::unique_ptr<InterpreterBuilderInterface>) std::make_unique<TfLiteInterpreterBuilder>("/tmp/mobilenet_v1_1.0_224.tflite");

    auto interpreter = interpreter_builder->BuildInterpreter();
    if(!interpreter)
    {
        std::cerr << "Failed to create interpreter";
        return -1;
    }

    auto input = interpreter->input_tensor(0);
    {
        std::string file_path("/home/dev/oosman/repos/edgetpu-minimal/models/cat.rgb");
        std::ifstream file(file_path, std::ios::binary);
        file.read((char*)input->data.data, input->bytes);
    }

    if(interpreter->Invoke() != kTfLiteOk)
    {
        std::cerr << "Failed to invoke model." << std::endl;
        return -1;
    }

    coral::GetClassificationResults(*interpreter, 0.0f, /*top_k=*/3);
    for (auto result : coral::GetClassificationResults(*interpreter, 0.0f, /*top_k=*/3))
    {
      std::cout << "---------------------------" << std::endl;
      //std::cout << labels[result.id] << std::endl;
      std::cout << "Score: " << result.score << ", ID: " << result.id << std::endl;
    }

//    tflite::PrintInterpreterState(interpreter.get());
    std::cout << "Done\n";
    return 0;
}
