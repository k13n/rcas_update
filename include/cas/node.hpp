#ifndef CAS_NODE_H_
#define CAS_NODE_H_

#include "cas/node_type.hpp"
#include "cas/index_stats.hpp"
#include <cstdint>
#include <cstring>
#include <vector>
#include <functional>
#include <fstream>


namespace cas {


class Node;
using ChildIt = std::function<bool(uint8_t, Node&)>;


class Node {
public:
  NodeType type_;
  uint16_t nr_children_ = 0;
  size_t nr_keys_ = 0;
  uint16_t separator_pos_ = 0;
  std::vector<uint8_t> prefix_;

  Node(NodeType type);

  virtual ~Node() = default;

  virtual inline bool IsLeaf() {
    return false;
  };

  bool IsPathNode();

  bool IsValueNode();

  NodeType Type();

  virtual bool IsFull() = 0;
  virtual bool IsUnderfilled() = 0;

  /**
   * Assumes key_byte is not yet present in node
   **/
  virtual void Put(uint8_t key_byte, Node *child) = 0;

  virtual Node* LocateChild(uint8_t key_byte) = 0;

  virtual Node* Grow() = 0;

  virtual Node* Shrink() = 0;

  virtual void ReplaceBytePointer(uint8_t key_byte, Node* child) = 0;

  virtual void ForEachChild(uint8_t low, uint8_t high, ChildIt callback) = 0;

  virtual void ForEachChild(uint8_t low, ChildIt callback);

  virtual void ForEachChild(ChildIt callback);

  virtual size_t SizeBytes() = 0;

  virtual void Dump();

  void DumpRecursive();

  void DumpConcise(uint8_t edge_label, int indent);

  void DumpConciseToFile(uint8_t edge_label, int indent, std::string file_name);

  void DumpLatex(Node* parent, uint8_t edge_label, int indent,
                 bool string_value);

  void CollectStats(IndexStats& stats, size_t depth);

  virtual int NodeWidth() = 0;

  size_t PathPrefixSize();

  size_t ValuePrefixSize();

  virtual std::vector<uint8_t> GetKeys();

  virtual void DeleteNode(uint8_t key_byte) = 0;

  uint8_t* Path() {
    return &prefix_[0];
  }

  uint8_t* Value() {
    return &prefix_[separator_pos_];
  }

  uint8_t* Prefix(NodeType type) {
    switch (type) {
      case cas::NodeType::Path:  return Path();
      case cas::NodeType::Value: return Value();
      case cas::NodeType::Leaf:
        throw std::runtime_error{"invalid node type"};
    }
    return nullptr;
  }

  size_t PrefixLen(NodeType type) {
    switch (type) {
      case cas::NodeType::Path:  return PathPrefixSize();
      case cas::NodeType::Value: return ValuePrefixSize();
      case cas::NodeType::Leaf:
        throw std::runtime_error{"invalid node type"};
    }
    return 0;
  }

protected:
  void DumpBuffer(uint8_t *buffer, size_t length);

  void DumpAddresses(Node **buffer, size_t length);

};


} // namespace cas

#endif  // CAS_NODE_H_
