// Copyright (c) 2018 PaddlePaddle Authors. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include <glog/logging.h>
#if !defined(_WIN32)
#include <sys/time.h>
#else
#endif

#include <algorithm>
#include <chrono>  // NOLINT
#include <iterator>
#include <numeric>
#include <sstream>
#include <string>
#include <vector>
#include "paddle/fluid/inference/api/paddle_inference_api.h"
#include "paddle/fluid/string/printf.h"

namespace paddle {
namespace inference {

// Timer for timer
class Timer {
 public:
  std::chrono::high_resolution_clock::time_point start;
  std::chrono::high_resolution_clock::time_point startu;

  void tic() { start = std::chrono::high_resolution_clock::now(); }
  double toc() {
    startu = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> time_span =
        std::chrono::duration_cast<std::chrono::duration<double>>(startu -
                                                                  start);
    double used_time_ms = static_cast<double>(time_span.count()) * 1000.0;
    return used_time_ms;
  }
};

static void split(const std::string &str, char sep,
                  std::vector<std::string> *pieces) {
  pieces->clear();
  if (str.empty()) {
    return;
  }
  size_t pos = 0;
  size_t next = str.find(sep, pos);
  while (next != std::string::npos) {
    pieces->push_back(str.substr(pos, next - pos));
    pos = next + 1;
    next = str.find(sep, pos);
  }
  if (!str.substr(pos).empty()) {
    pieces->push_back(str.substr(pos));
  }
}
static void split_to_float(const std::string &str, char sep,
                           std::vector<float> *fs) {
  std::vector<std::string> pieces;
  split(str, sep, &pieces);
  std::transform(pieces.begin(), pieces.end(), std::back_inserter(*fs),
                 [](const std::string &v) { return std::stof(v); });
}
static void split_to_int64(const std::string &str, char sep,
                           std::vector<int64_t> *is) {
  std::vector<std::string> pieces;
  split(str, sep, &pieces);
  std::transform(pieces.begin(), pieces.end(), std::back_inserter(*is),
                 [](const std::string &v) { return std::stoi(v); });
}
template <typename T>
std::string to_string(const std::vector<T> &vec) {
  std::stringstream ss;
  for (const auto &c : vec) {
    ss << c << " ";
  }
  return ss.str();
}
template <>
std::string to_string<std::vector<float>>(
    const std::vector<std::vector<float>> &vec);

template <>
std::string to_string<std::vector<std::vector<float>>>(
    const std::vector<std::vector<std::vector<float>>> &vec);

template <typename T>
int VecReduceToInt(const std::vector<T> &v) {
  return std::accumulate(v.begin(), v.end(), 1, [](T a, T b) { return a * b; });
}

template <typename T>
static void TensorAssignData(PaddleTensor *tensor,
                             const std::vector<std::vector<T>> &data) {
  // Assign buffer
  int num_elems = VecReduceToInt(tensor->shape);
  tensor->data.Resize(sizeof(T) * num_elems);
  int c = 0;
  for (const auto &f : data) {
    for (T v : f) {
      static_cast<T *>(tensor->data.data())[c++] = v;
    }
  }
}

template <typename T>
static int ZeroCopyTensorAssignData(ZeroCopyTensor *tensor,
                                    const std::vector<std::vector<T>> &data) {
  int size{0};
  auto *ptr = tensor->mutable_data<T>(PaddlePlace::kCPU);
  int c = 0;
  for (const auto &f : data) {
    for (T v : f) {
      ptr[c++] = v;
    }
  }
  return size;
}

static bool CompareTensor(const PaddleTensor &a, const PaddleTensor &b) {
  if (a.dtype != b.dtype) {
    LOG(ERROR) << "dtype not match";
    return false;
  }

  if (a.lod.size() != b.lod.size()) {
    LOG(ERROR) << "lod not match";
    return false;
  }
  for (size_t i = 0; i < a.lod.size(); i++) {
    if (a.lod[i].size() != b.lod[i].size()) {
      LOG(ERROR) << "lod not match";
      return false;
    }
    for (size_t j = 0; j < a.lod[i].size(); j++) {
      if (a.lod[i][j] != b.lod[i][j]) {
        LOG(ERROR) << "lod not match";
        return false;
      }
    }
  }

  if (a.shape.size() != b.shape.size()) {
    LOG(INFO) << "shape not match";
    return false;
  }
  for (size_t i = 0; i < a.shape.size(); i++) {
    if (a.shape[i] != b.shape[i]) {
      LOG(ERROR) << "shape not match";
      return false;
    }
  }

  auto *adata = static_cast<float *>(a.data.data());
  auto *bdata = static_cast<float *>(b.data.data());
  for (int i = 0; i < VecReduceToInt(a.shape); i++) {
    if (adata[i] != bdata[i]) {
      LOG(ERROR) << "data not match";
      return false;
    }
  }
  return true;
}

static std::string DescribeTensor(const PaddleTensor &tensor) {
  std::stringstream os;
  os << "Tensor [" << tensor.name << "]\n";
  os << " - type: ";
  switch (tensor.dtype) {
    case PaddleDType::FLOAT32:
      os << "float32";
      break;
    case PaddleDType::INT64:
      os << "int64";
      break;
    default:
      os << "unset";
  }
  os << '\n';

  os << " - shape: " << to_string(tensor.shape) << '\n';
  os << " - lod: ";
  for (auto &l : tensor.lod) {
    os << to_string(l) << "; ";
  }
  os << "\n";
  os << " - data: ";

  int dim = VecReduceToInt(tensor.shape);
  for (int i = 0; i < dim; i++) {
    os << static_cast<float *>(tensor.data.data())[i] << " ";
  }
  os << '\n';
  return os.str();
}

static std::string DescribeZeroCopyTensor(const ZeroCopyTensor &tensor) {
  std::stringstream os;
  os << "Tensor [" << tensor.name() << "]\n";

  os << " - shape: " << to_string(tensor.shape()) << '\n';
  os << " - lod: ";
  for (auto &l : tensor.lod()) {
    os << to_string(l) << "; ";
  }
  os << "\n";
  os << " - data: ";
  PaddlePlace place;
  int size;
  const auto *data = tensor.data<float>(&place, &size);
  for (int i = 0; i < size; i++) {
    os << data[i] << " ";
  }
  return os.str();
}

static void PrintTime(int batch_size, int repeat, int num_threads, int tid,
                      double latency, int epoch = 1) {
  LOG(INFO) << "====== batch_size: " << batch_size << ", repeat: " << repeat
            << ", threads: " << num_threads << ", thread id: " << tid
            << ", latency: " << latency << "ms, fps: " << 1 / (latency / 1000.f)
            << " ======";
  if (epoch > 1) {
    int samples = batch_size * epoch;
    LOG(INFO) << "====== sample number: " << samples
              << ", average latency of each sample: " << latency / samples
              << "ms ======";
  }
}

}  // namespace inference
}  // namespace paddle
