// Copyright 2017 Google Inc. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS-IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

// Author: ericv@google.com (Eric Veach)

#include "s2/s2boolean_operation.h"

#include <memory>
#include <gtest/gtest.h>
#include "s2/third_party/absl/memory/memory.h"
#include "s2/third_party/absl/strings/str_split.h"
#include "s2/third_party/absl/strings/strip.h"
#include "s2/mutable_s2shape_index.h"
#include "s2/s2builder.h"
#include "s2/s2builder_graph.h"
#include "s2/s2builder_layer.h"
#include "s2/s2builderutil_lax_polygon_layer.h"
#include "s2/s2builderutil_s2point_vector_layer.h"
#include "s2/s2builderutil_s2polyline_vector_layer.h"
#include "s2/s2builderutil_snap_functions.h"
#include "s2/s2polygon.h"
#include "s2/s2text_format.h"

namespace {

using absl::make_unique;
using s2builderutil::LaxPolygonLayer;
using std::unique_ptr;
using std::vector;

using Graph = S2Builder::Graph;
using GraphOptions = S2Builder::GraphOptions;
using DegenerateEdges = GraphOptions::DegenerateEdges;
using DuplicateEdges = GraphOptions::DuplicateEdges;
using SiblingPairs = GraphOptions::SiblingPairs;

using OpType = S2BooleanOperation::OpType;
using PolygonModel = S2BooleanOperation::PolygonModel;
using PolylineModel = S2BooleanOperation::PolylineModel;

using DegenerateBoundaries = LaxPolygonLayer::Options::DegenerateBoundaries;

S2Error::Code INDEXES_DO_NOT_MATCH = S2Error::USER_DEFINED_START;

class IndexMatchingLayer : public S2Builder::Layer {
 public:
  explicit IndexMatchingLayer(const S2ShapeIndex* index, int dimension)
      : index_(*index), dimension_(dimension) {
  }
  GraphOptions graph_options() const override {
    return GraphOptions(EdgeType::DIRECTED, DegenerateEdges::KEEP,
                        DuplicateEdges::KEEP, SiblingPairs::KEEP);
  }

  void Build(const Graph& g, S2Error* error) override;

 private:
  using EdgeVector = vector<S2Shape::Edge>;
  static string ToString(const EdgeVector& edges);

  const S2ShapeIndex& index_;
  int dimension_;
};

string IndexMatchingLayer::ToString(const EdgeVector& edges) {
  string msg;
  for (const auto& edge : edges) {
    vector<S2Point> vertices{edge.v0, edge.v1};
    msg += s2textformat::ToString(vertices);
    msg += "; ";
  }
  return msg;
}

void IndexMatchingLayer::Build(const Graph& g, S2Error* error) {
  vector<S2Shape::Edge> actual, expected;
  for (int e = 0; e < g.num_edges(); ++e) {
    const Graph::Edge& edge = g.edge(e);
    actual.push_back(S2Shape::Edge(g.vertex(edge.first),
                                   g.vertex(edge.second)));
  }
  for (S2Shape* shape : index_) {
    if (shape == nullptr || shape->dimension() != dimension_) {
      continue;
    }
    for (int e = shape->num_edges(); --e >= 0; ) {
      expected.push_back(shape->edge(e));
    }
  }
  std::sort(actual.begin(), actual.end());
  std::sort(expected.begin(), expected.end());

  // The edges are a multiset, so we can't use std::set_difference.
  vector<S2Shape::Edge> missing, extra;
  for (auto ai = actual.begin(), ei = expected.begin();
       ai != actual.end() || ei != expected.end(); ) {
    if (ei == expected.end() || (ai != actual.end() && *ai < *ei)) {
      extra.push_back(*ai++);
    } else if (ai == actual.end() || *ei < *ai) {
      missing.push_back(*ei++);
    } else {
      ++ai;
      ++ei;
    }
  }
  if (!missing.empty() || !extra.empty()) {
    // There may be errors in more than one dimension, so we append to the
    // existing error text.
    error->Init(INDEXES_DO_NOT_MATCH,
                "%sDimension %d: Missing edges: %s Extra edges: %s\n",
                error->text().c_str(), dimension_, ToString(missing).c_str(),
                ToString(extra).c_str());
  }
}

void ExpectResult(S2BooleanOperation::OpType op_type,
                  const S2BooleanOperation::Options& options,
                  const string& a_str, const string& b_str,
                  const string& expected_str) {
  auto a = s2textformat::MakeIndexOrDie(a_str);
  auto b = s2textformat::MakeIndexOrDie(b_str);
  auto expected = s2textformat::MakeIndexOrDie(expected_str);
  vector<unique_ptr<S2Builder::Layer>> layers;
  for (int dim = 0; dim < 3; ++dim) {
    layers.push_back(make_unique<IndexMatchingLayer>(expected.get(), dim));
  }
  S2BooleanOperation op(op_type, std::move(layers), options);
  S2Error error;
  EXPECT_TRUE(op.Build(*a, *b, &error))
      << S2BooleanOperation::OpTypeToString(op_type) << " failed:\n"
      << "Expected result: " << expected_str << "\n" << error;

  // Now try the same thing with boolean output.
  EXPECT_EQ(expected->num_shape_ids() == 0,
            S2BooleanOperation::IsEmpty(op_type, *a, *b, options));
}

}  // namespace

// The intersections in the "expected" data below were computed in lat-lng
// space (i.e., the rectangular projection), while the actual intersections
// are computed using geodesics.  We can compensate for this by rounding the
// intersection points to a fixed precision in degrees (e.g., 2 decimals).
static S2BooleanOperation::Options RoundToE(int exp) {
  S2BooleanOperation::Options options;
  options.set_snap_function(s2builderutil::IntLatLngSnapFunction(exp));
  return options;
}

