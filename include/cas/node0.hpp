#ifndef CAS_NODE0_H_
#define CAS_NODE0_H_

#include "cas/types.hpp"
#include "cas/binary_key.hpp"
#include "cas/interleaved_key.hpp"
#include "cas/node.hpp"

namespace cas {


class Node0 : public Node {
public:
  std::vector<did_t> dids_;

  Node0();

  Node0(const BinaryKey& bkey, size_t path_pos, size_t value_pos);

  Node0(const InterleavedKey& ikey, size_t pos);

  inline bool IsLeaf() {
    return true;
  };

  inline bool IsFull() {
    return true;
  }

  void Put(uint8_t key_byte, Node *child);

  Node* LocateChild(uint8_t key_byte);

  inline Node* Grow() {
    return nullptr;
  }

  void ReplaceBytePointer(uint8_t key_byte, Node* child);

  void ForEachChild(uint8_t low, uint8_t high, ChildIt callback);

  size_t SizeBytes();

  void Dump();

  bool ContainsDid(did_t did);

  int NodeWidth();

  std::vector<uint8_t> GetKeys();

  void DeleteNode(uint8_t key_byte);

};


} // namespace cas

#endif  // CAS_NODE0_H_
