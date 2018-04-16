//   Copyright (c) 2018 PaddlePaddle Authors. All Rights Reserved.
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
#include <sstream>
#include <string>
#include <unordered_set>

#include "paddle/fluid/platform/place.h"

namespace paddle {
namespace framework {
namespace details {
class OpHandleBase;

// VarHandleBase is the var node in the dependency graph.
// A variable can only be generated by a single operator. i.e.
// This is a single assignment graph.
struct VarHandleBase {
  virtual ~VarHandleBase();
  virtual std::string DebugString() const = 0;

  // The operator who generate this variable. nullptr if the variable
  // is a root node.
  OpHandleBase *generated_op_;

  // Operators which depend on this variable ready.
  std::unordered_set<OpHandleBase *> pending_ops_;
};

// VarHandle is actually a single version of Runtime Variable.
// Variable in Runtime mapped to many VarHandles in Graph.
// Each assignment will generate a new var handle with newer version.
//
// NOTE: runtime variables have place.
struct VarHandle : public VarHandleBase {
  std::string DebugString() const override;

  // version field currently is not used, however, just store the version to
  // debug easily.
  size_t version_;
  size_t scope_idx_;
  std::string name_;
  platform::Place place_;
};

// Dummy Variable. It is used to represent dependencies between operators
struct DummyVarHandle : public VarHandleBase {
  std::string DebugString() const override;
};

}  // namespace details
}  // namespace framework
}  // namespace paddle
