#include "adapter.h"

#include <algorithm>
#include <iostream>
#include <numeric>
#include <queue>
#include <tuple>

#include "absl/strings/substitute.h"
//#include "tflite_utils.h"
//#include "glog/logging.h"

namespace coral {
namespace {
// Defines a comparator which allows us to rank Class based on their score and
// id.
struct ClassComparator {
  bool operator()(const Class& lhs, const Class& rhs) const {
    return std::tie(lhs.score, lhs.id) > std::tie(rhs.score, rhs.id);
  }
};
struct ObjectComparator {
  bool operator()(const Object& lhs, const Object& rhs) const {
    return std::tie(lhs.score, lhs.id) > std::tie(rhs.score, rhs.id);
  }
};
inline bool operator==(const Object& x, const Object& y) {
  return x.id == y.id && x.score == y.score && x.bbox == y.bbox;
}

inline bool operator!=(const Object& x, const Object& y) { return !(x == y); }

inline absl::Span<const int> TensorShape(const TfLiteTensor& tensor) {
  return absl::Span<const int>(tensor.dims->data, tensor.dims->size);
}

std::string ToString(const Object& obj) {
  return absl::Substitute("Object(id=$0,score=$1,bbox=$2)", obj.id, obj.score,
                          ToString(obj.bbox));
}

inline std::ostream& operator<<(std::ostream& os, const Object& obj) {
  return os << ToString(obj);
}

inline int TensorSize(const TfLiteTensor& tensor) {
  auto shape = TensorShape(tensor);
  return std::accumulate(shape.begin(), shape.end(), 1, std::multiplies<int>());
}
template <typename T>
absl::Span<const T> TensorData(const TfLiteTensor& tensor) {
  return absl::MakeSpan(reinterpret_cast<const T*>(tensor.data.data),
                        tensor.bytes / sizeof(T));
}
template <typename InputIt, typename OutputIt>
OutputIt Dequantize(InputIt first, InputIt last, OutputIt d_first, float scale,
                    int32_t zero_point) {
  while (first != last) *d_first++ = scale * (*first++ - zero_point);
  return d_first;
}

template <typename T, typename OutputIt>
OutputIt Dequantize(absl::Span<const T> span, OutputIt d_first, float scale,
                    int32_t zero_point) {
  return Dequantize(span.begin(), span.end(), d_first, scale, zero_point);
}
template <typename T>
std::vector<T> DequantizeTensor(const TfLiteTensor& tensor) {
  const auto scale = tensor.params.scale;
  const auto zero_point = tensor.params.zero_point;
  std::vector<T> result(TensorSize(tensor));

  if (tensor.type == kTfLiteUInt8)
    Dequantize(TensorData<uint8_t>(tensor), result.begin(), scale, zero_point);
  else if (tensor.type == kTfLiteInt8)
    Dequantize(TensorData<int8_t>(tensor), result.begin(), scale, zero_point);
  else {
//    LOG(FATAL)
      std::cerr << "Unsupported tensor type: " << tensor.type;
      exit(-1);
    }
  return result;
}

//osm done
}  // namespace

std::string ToString(const Class& c) {
  return absl::Substitute("Class(id=$0,score=$1)", c.id, c.score);
}

std::vector<Class> GetClassificationResults(absl::Span<const float> scores,
                                            float threshold, size_t top_k) {
  std::priority_queue<Class, std::vector<Class>, ClassComparator> q;
  for (int i = 0; i < scores.size(); ++i) {
    if (scores[i] < threshold) continue;
    q.push(Class{i, scores[i]});
    if (q.size() > top_k) q.pop();
  }

  std::vector<Class> ret;
  while (!q.empty()) {
    ret.push_back(q.top());
    q.pop();
  }
  std::reverse(ret.begin(), ret.end());
  return ret;
}

std::vector<Class> GetClassificationResults(
    const tflite::Interpreter& interpreter, float threshold, size_t top_k) {
  const auto& tensor = *interpreter.output_tensor(0);
  if (tensor.type == kTfLiteUInt8 || tensor.type == kTfLiteInt8) {
        return GetClassificationResults(DequantizeTensor<float>(tensor), threshold,
                                        top_k);
  } else if (tensor.type == kTfLiteFloat32) {
    return GetClassificationResults(TensorData<float>(tensor), threshold,
                                    top_k);
  } else {
//    LOG(FATAL)
      std::cerr << "Unsupported tensor type: " << tensor.type;
      exit(-1);
  }
}

/////////////////////////////////////////////////////////////////////////////////////////
std::vector<Object> GetDetectionResults(absl::Span<const float> bboxes,
                                        absl::Span<const float> ids,
                                        absl::Span<const float> scores,
                                        size_t count, float threshold,
                                        size_t top_k) {
  if (bboxes.size() % 4 != 0) { exit(-1); }
  if (4 * count > bboxes.size()) { exit(-1); }
  if (count > ids.size()) { exit(-1); }
  if (count > scores.size()) { exit(-1); }

  std::priority_queue<Object, std::vector<Object>, ObjectComparator> q;
  for (int i = 0; i < count; ++i) {
    const int id = std::round(ids[i]);
    const float score = scores[i];
    if (score < threshold) continue;
    const float ymin = std::max(0.0f, bboxes[4 * i]);
    const float xmin = std::max(0.0f, bboxes[4 * i + 1]);
    const float ymax = std::min(1.0f, bboxes[4 * i + 2]);
    const float xmax = std::min(1.0f, bboxes[4 * i + 3]);
    q.push(Object{id, score, BBox<float>{ymin, xmin, ymax, xmax}});
    if (q.size() > top_k) q.pop();
  }

  std::vector<Object> ret;
  ret.reserve(q.size());
  while (!q.empty()) {
    ret.push_back(q.top());
    q.pop();
  }
  std::reverse(ret.begin(), ret.end());
  return ret;
}

std::vector<Object> GetDetectionResults(const tflite::Interpreter& interpreter,
                                        float threshold, size_t top_k) {
  if (interpreter.outputs().size() != 4)
  {
      std::cerr << "Expecting 4 outputs, but received " << interpreter.outputs().size() << "\n";
      exit(-1);
  }
  auto bboxes = TensorData<float>(*interpreter.output_tensor(0));
  auto ids = TensorData<float>(*interpreter.output_tensor(1));
  auto scores = TensorData<float>(*interpreter.output_tensor(2));
  auto count = TensorData<float>(*interpreter.output_tensor(3));
  if (bboxes.size() != 4 * ids.size()) { exit(-1); }
  if (bboxes.size() != 4 * scores.size()) { exit(-1); }
  if (count.size() != 1) { exit(-1); }
  return GetDetectionResults(bboxes, ids, scores, static_cast<size_t>(count[0]),
                             threshold, top_k);
}

}  // namespace coral
