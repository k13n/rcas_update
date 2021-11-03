#ifndef CAS_NODE256_H_
#define CAS_NODE256_H_

#include "cas/node.hpp"


namespace cas {


class Node256 : public Node {
public:
  Node* children_[256];

  Node256(NodeType type);

  bool IsFull();
  bool IsUnderfilled();

  void Put(uint8_t key_byte, Node* child);

  Node* LocateChild(uint8_t key_byte);

  Node* Grow();

  Node* Shrink();

  void ReplaceBytePointer(uint8_t key_byte, Node* child);

  void ForEachChild(uint8_t low, uint8_t high, ChildIt callback);

  size_t SizeBytes();

  void Dump();

  int NodeWidth();

  std::vector<uint8_t> GetKeys();

  void DeleteNode(uint8_t key_byte);
};


} // namespace cas

#endif  // CAS_NODE256_H_
