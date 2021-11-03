#ifndef CAS_NODE16_H_
#define CAS_NODE16_H_

#include "cas/node.hpp"


namespace cas {


class Node16 : public Node {
public:
  uint8_t keys_[16];
  Node* children_[16];

  Node16(NodeType type);

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

#endif  // CAS_NODE16_H_
