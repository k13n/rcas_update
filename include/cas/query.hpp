#ifndef CAS_QUERY_H_
#define CAS_QUERY_H_

#include "cas/node.hpp"
#include "cas/path_matcher.hpp"
#include "cas/query_stats.hpp"
#include "cas/key_encoding.hpp"
#include "cas/search_key.hpp"
#include "cas/index.hpp"

#include <deque>


namespace cas {


template<class VType>
class Query {
  struct State {
    Node* node_; //current node that is being traversed
    NodeType parent_type_;
    uint8_t parent_byte_;
    uint16_t len_pat_;
    uint16_t len_val_;
    PathMatcher::State pm_state_;
    uint16_t vl_pos_;
    uint16_t vh_pos_;

    void Dump();
  };

  Node* root_;
  BinarySK& key_;
  PathMatcher& pm_;
  BinaryKeyEmitter emitter_;
  std::vector<uint8_t> buf_pat_;
  std::vector<uint8_t> buf_val_;
  std::deque<State> stack_;
  QueryStats stats_;

  Node* auxiliary_index_;

public:
  Query(Node* root, BinarySK& key, cas::PathMatcher& pm,
      BinaryKeyEmitter emitter);

  void Execute();

  const QueryStats& Stats() const {
    return stats_;
  }

  void setAuxiliaryIndex(Node *node);

private:
  void PrepareBuffer(State& s);

  void PrepareBufferAuxiliaryIndex(State& s);

  PathMatcher::PrefixMatch MatchPathPrefix(State& s);

  PathMatcher::PrefixMatch MatchValuePrefix(State& s);

  void Descend(State& s);

  void DescendPathNode(State& s);

  void DescendValueNode(State& s);

  bool IsCompleteValue(State& s);

  void EmitMatch(State& s);

  void UpdateStats(State& s);

  void DumpState(State& s);

};


} // namespace cas

#endif // CAS_QUERY_H_
