/* Copyright (c) 2018 PaddlePaddle Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License. */

#include "paddle/fluid/framework/ir/pass.h"

#include <memory>
#include <utility>

#include "paddle/fluid/framework/ir/graph_helper.h"

namespace paddle {
namespace framework {
namespace ir {
std::unique_ptr<Graph> Pass::Apply(std::unique_ptr<Graph> graph) const {
  VLOG(3) << "apply pass -> " << Type();
  PADDLE_ENFORCE(graph.get(), "graph passed to Pass::Apply() cannot be empty.");
  for (const std::string& attr : required_pass_attrs_) {
    PADDLE_ENFORCE(attrs_.find(attr) != attrs_.end(),
                   "Required pass atrribute %s not set.", attr);
  }
  for (const std::string& attr : required_graph_attrs_) {
    PADDLE_ENFORCE(graph->Has(attr), "Required graph atrribute %s not set.",
                   attr);
  }
  auto* native_graph = graph.get();
  auto applied_graph = ApplyImpl(std::move(graph));
  // TODO(panyx0718): Add more verifications.
  PADDLE_ENFORCE(!HasCircle(*applied_graph),
                 "Illegal Pass. Generated graph shouldn't has cycle.");
  PADDLE_ENFORCE(applied_graph.get() == native_graph,
                 "Pass::Apply() cannot delete the passed graph and shouldn't "
                 "return a new graph.(For the need of pybind11)");
  applied_ = true;
  return applied_graph;
}

PassRegistry& PassRegistry::Instance() {
  static PassRegistry g_pass_info_map;
  return g_pass_info_map;
}
}  // namespace ir
}  // namespace framework
}  // namespace paddle
