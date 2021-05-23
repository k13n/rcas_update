#ifndef CAS_INTERLEAVING_H
#define CAS_INTERLEAVING_H

#include "cas/cas.hpp"
#include "cas/binary_key.hpp"
#include "cas/node.hpp"
#include "cas/node_type.hpp"

#include <deque>
#include <vector>

namespace cas {


const std::string COL_BLUE = "\033[94m";
const std::string COL_RED  = "\033[31m";
const std::string COL_NONE = "\033[0m";


template<class VType>
class Interleaving {
  struct Tuple {
    std::vector<uint8_t> path_bytes_;
    std::vector<uint8_t> value_bytes_;
    cas::NodeType dimension_;
  };

  cas::Cas<VType>& index_;
  cas::BinaryKey& key_;
  Node* node_;
  std::vector<Tuple> result_;
  size_t p_pos_ = 0;
  size_t v_pos_ = 0;

public:
  Interleaving(cas::Cas<VType>& index, cas::BinaryKey& key);

  void Compute();

private:
  bool Descend();

  bool MatchPath(size_t& pos);

  bool MatchValue(size_t& pos);

  void DumpInterleaving();

  void DumpInterleavingLatex();
};


} // namespace cas

#endif // CAS_INTERLEAVING_H
