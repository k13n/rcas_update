#ifndef CAS_CAS_INSERT_H_
#define CAS_CAS_INSERT_H_

#include "cas/node.hpp"
#include "cas/insertion_helper.hpp"
#include "cas/query_stats.hpp"
#include "cas/key_encoding.hpp"
#include "cas/search_key.hpp"
#include "cas/index.hpp"
#include "binary_key.hpp"
#include "insert_type.hpp"

#include <deque>
#include <stack>

namespace cas {


template<class VType>
class CasInsert {
  struct State {
    Node* node_; //current node that is being traversed
    NodeType parent_type_;
    uint8_t parent_byte_;
    uint16_t len_pat_;
    uint16_t len_val_;
    InsertionHelper::State pm_state_; //state needed for the path matching
    uint16_t vl_pos_;
    uint16_t vh_pos_;

    void Dump();
  };

  Node* root_;
  Node* parent_;
  Node* grand_parent_; //grand_parent_ is necessary to know when the node is expanded
  BinarySK& key_;
  InsertionHelper& pm_;
  did_t did_;
  std::vector<uint8_t> buf_pat_;
  std::vector<uint8_t> buf_val_;
  std::deque<State> stack_;
  QueryStats stats_;
  Node* second_index_; //second(auxiliary) index
  MergeMethod merge_method_;


public:
    Node *getSecondIndex() const;

private:
    bool is_main_index_;

public:
  CasInsert(
      Node* root,
      BinarySK& key,
      cas::InsertionHelper& pm,
      cas::did_t did,
      Node* second_index,
      bool isMain,
      cas::MergeMethod merge_method = cas::MergeMethod::Slow);

  bool Execute(cas::Node*& root_node, cas::InsertType insertType, cas::Node*& root_node_sec);

  const QueryStats& Stats() const {
    return stats_;
  }

  void mergeIndexes(
      cas::Node* node_prim,
      cas::Node* node_sec,
      cas::Node* parent_node_prim,
      cas::Node* parent_node_sec,
      uint8_t parent_byte_prim,
      uint8_t parent_byte_sec,
      NodeType parent_type_prim,
      NodeType parent_type_sec,
      std::stack<cas::Node*> traversed_nodes_sec);

  void collectMainAndAuxIndexAndRebuildSubtree(
      cas::Node* node_prim,
      cas::Node* node_sec,
      cas::Node* parent_node_prim,
      cas::Node* parent_node_sec,
      uint8_t parent_byte_prim,
      uint8_t parent_byte_sec,
      NodeType parent_type_prim,
      NodeType parent_type_sec,
      std::stack<cas::Node*> traversed_nodes_sec);

private:
  void PrepareBuffer(State& s);

  InsertionHelper::State InsertionMatchPathPrefix(State& s);

  void InsertionMatchValuePrefix(State& s);

  void Descend(State& s);

  void DescendPathNode(State& s);

  void DescendValueNode(State& s);

  void StrictSlowInsertion(State& s, uint16_t next_node_qpos_, uint16_t next_node_vl_pos_);

  void CollectSubtreeKeys(std::deque<cas::BinaryKey>& bkeys_,
          cas::Node* node,
          std::vector<uint8_t> path_prefix_,
          std::vector<uint8_t> value_prefix_,
          int& leaf_counter);

  cas::NodeType DetermineDimension(State& s, uint16_t ip, uint16_t iv);

  void LazyFastInsertion(State& s, uint16_t next_node_qpos_, uint16_t next_node_vl_pos_);

  bool IsCompleteValue(State& s);

  void UpdateStats(State& s);

  void DumpState(State& s);

  void DeleteTreeNodesRecursively(Node *node, int& delete_counter);
};

} // namespace cas

#endif // CAS_CAS_INSERT_H_
