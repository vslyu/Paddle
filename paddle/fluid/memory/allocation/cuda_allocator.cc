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

#include "paddle/fluid/memory/allocation/cuda_allocator.h"
#include <cuda.h>
#include <cuda_runtime.h>
#include <string>
#include "paddle/fluid/platform/gpu_info.h"

namespace paddle {
namespace memory {
namespace allocation {

class CUDADeviceGuard {
 public:
  explicit CUDADeviceGuard(int dev_id) {
    int prev_id = platform::GetCurrentDeviceId();
    if (prev_id != dev_id) {
      prev_id_ = prev_id;
      platform::SetDeviceId(dev_id);
    }
  }

  ~CUDADeviceGuard() {
    if (prev_id_ != -1) {
      platform::SetDeviceId(prev_id_);
    }
  }

 private:
  int prev_id_{-1};
};

std::unique_ptr<Allocation> CUDAAllocator::Allocate(size_t size, Attr attr) {
  CUDADeviceGuard guard(place_.device);
  void* ptr;
  auto status = cudaMalloc(&ptr, size);
  if (UNLIKELY(status != cudaSuccess)) {
    throw BadAlloc(string::Sprintf(
        "Cannot allocate %d on GPU %d, cuda status %d, %s", size, place_.device,
        status, cudaGetErrorString(status)));
  }

  return std::unique_ptr<Allocation>(
      new CUDAAllocation(ptr, size, platform::Place(place_)));
}

void CUDAAllocator::Free(Allocation* allocation) {
  auto* cuda_allocation = dynamic_cast<CUDAAllocation*>(allocation);
  PADDLE_ENFORCE_NOT_NULL(cuda_allocation);
  PADDLE_ENFORCE_EQ(boost::get<platform::CUDAPlace>(cuda_allocation->place()),
                    place_);
  PADDLE_ENFORCE(cudaFree(allocation->ptr()));
}
bool CUDAAllocator::IsAllocThreadSafe() const { return true; }
}  // namespace allocation
}  // namespace memory
}  // namespace paddle
