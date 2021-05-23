#ifndef CAS_INTERLEAVING_SCORE_H
#define CAS_INTERLEAVING_SCORE_H

#include "cas/cas.hpp"
#include "cas/index_type.hpp"
#include "cas/node_type.hpp"
#include "cas/node.hpp"

#include <deque>
#include <vector>

namespace cas {


template<class VType>
class InterleavingScore {
  struct State {
    Node* node_;
    size_t depth_;
  };

  const size_t MAX_DEPTH_ = 1000;

  cas::Cas<VType>& index_;
  std::deque<State> stack_;
  std::vector<cas::NodeType> type_vector_;
  double sum_scores_ = 0;
  size_t nr_keys_ = 0;

public:
  InterleavingScore(cas::Cas<VType>& index);

  double Compute();

private:
  void Descend(State& s);

  void ComputeKeyScore(State& s);

  double InterleavingRatio(std::vector<cas::NodeType>& phi);

  double InterleavingRatioExp(std::vector<cas::NodeType>& phi);

  void DumpPhi(std::vector<cas::NodeType>& phi);
};


} // namespace cas

#endif // CAS_INTERLEAVING_SCORE_H
