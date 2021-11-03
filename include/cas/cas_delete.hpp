#ifndef CAS_CAS_DELETE_H_
#define CAS_CAS_DELETE_H_

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
class CasDelete {
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

  Node** root_main_;
  Node** root_auxiliary_;
  Node* node_;
  Node* parent_;
  Node* grand_parent_;
  uint8_t parent_byte_; // byte from parent to node
  uint8_t grand_parent_byte_; // byte from grand_parent to parent
  const BinaryKey& key_;

private:
    bool is_main_index_;

public:
  CasDelete(Node** root_main, Node** root_auxiliary, const BinaryKey& key);

  bool Execute();
  bool Execute(Node** root);
  void LazyDeletion(Node** root);
  void StrictDeletion(Node** root);

private:
  bool Traverse(Node** root);
  void DeleteDID(std::vector<did_t>& v, cas::did_t did);
};

} // namespace cas

#endif // CAS_CAS_DELETE_H_
