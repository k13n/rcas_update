#include "cas/interleaving_score.hpp"
#include "cas/key_encoding.hpp"
#include "cas/interleaved_key.hpp"
#include "cas/interleaver.hpp"
#include "cas/node0.hpp"

#include <iostream>
#include <cassert>
#include <cmath>


template<class VType>
cas::InterleavingScore<VType>::InterleavingScore(cas::Cas<VType>& index)
  : index_(index) {
  type_vector_.reserve(MAX_DEPTH_);
}


template<class VType>
double cas::InterleavingScore<VType>::Compute() {
  stack_.push_back({
    .node_  = index_.root_,
    .depth_ = 0,
  });

  while (!stack_.empty()) {
    State s = stack_.back();
    stack_.pop_back();

    type_vector_[s.depth_] = s.node_->type_;
    if (s.node_->IsLeaf()) {
      ComputeKeyScore(s);
    } else {
      Descend(s);
    }
  }

  return sum_scores_ / nr_keys_;
}


template<class VType>
void cas::InterleavingScore<VType>::Descend(State& s) {
  s.node_->ForEachChild([&](uint8_t /*byte*/, cas::Node& child) -> bool {
    State child_s = {
      .node_        = &child,
      .depth_       = s.depth_ + 1,
    };
    stack_.push_back(child_s);
    return true;
  });
}


template<class VType>
void cas::InterleavingScore<VType>::ComputeKeyScore(State& s) {
  std::vector<cas::NodeType> phi;

  // the last node type is leaf node, don't count that
  for (size_t i = 0; i < s.depth_ - 1; ++i) {
    phi.push_back(type_vector_[i]);
  }

  double ratio = InterleavingRatio(phi);
  /* double ratio = InterleavingRatioExp(phi); */

  cas::Node0* leaf = static_cast<cas::Node0*>(s.node_);
  sum_scores_ += ratio * leaf->dids_.size();
  nr_keys_ += leaf->dids_.size();
}


template<class VType>
double cas::InterleavingScore<VType>::InterleavingRatio(
    std::vector<cas::NodeType>& phi) {
  if (phi.size() <= 1) {
    return 0;
  }
  int nr_changes = 0;
  for (size_t i = 1; i < phi.size(); ++i) {
    if (phi[i-1] != phi[i]) {
      ++nr_changes;
    }
  }
  return static_cast<double>(nr_changes) / (phi.size() - 1);
}


template<class VType>
double cas::InterleavingScore<VType>::InterleavingRatioExp(
    std::vector<cas::NodeType>& phi) {
  if (phi.size() <= 1) {
    return 0;
  }
  double lambda = 0.5;
  double numerator = 0;
  double denominator = 0;
  for (size_t i = 1; i < phi.size(); ++i) {
    double factor = std::exp((1-static_cast<double>(i))*lambda);
    if (phi[i-1] != phi[i]) {
      numerator += factor;
    }
    denominator += factor;
  }
  return numerator / denominator;
}


template<class VType>
void cas::InterleavingScore<VType>::DumpPhi(std::vector<cas::NodeType>& phi) {
  bool first = true;
  std::cout << "(";
  for (auto type : phi) {
    if (!first) {
      std::cout << ",";
    }
    first = false;
    switch (type) {
      case cas::NodeType::Path:  std::cout << "P"; break;
      case cas::NodeType::Value: std::cout << "V"; break;
      case cas::NodeType::Leaf:  assert(false); /* impossible */ break;
    }
  }
  std::cout << ")";
}


// explicit instantiations to separate header from implementation
template class cas::InterleavingScore<cas::vint32_t>;
template class cas::InterleavingScore<cas::vint64_t>;
template class cas::InterleavingScore<cas::vstring_t>;
