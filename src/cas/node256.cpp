#include "cas/node256.hpp"
#include <cassert>
#include <iostream>


cas::Node256::Node256(cas::NodeType type)
    : cas::Node(type) {
  memset(children_, 0, 256*sizeof(uintptr_t));
}


void cas::Node256::Put(uint8_t key_byte, Node* child) {
  if (children_[key_byte] != nullptr) {
    assert(false);
    return;
  }
  children_[key_byte] = child;
  ++nr_children_;
}


void cas::Node256::DeleteNode(uint8_t key_byte) {
  if (nr_children_ == 0) {
    // TODO
    std::cout<<"** You cannot delete from empty node 256**"<<std::endl;
    exit(-1);
  }
  //If key does not exists, it is error
  if(children_[key_byte] == nullptr){
    // TODO
    std::cout<<"** You cannot delete non existing node 256**"<<std::endl;
    exit(-1);
  }

  children_[key_byte] = nullptr;
  --nr_children_;
}


cas::Node* cas::Node256::LocateChild(uint8_t key_byte) {
  return children_[key_byte];
}


std::vector<uint8_t> cas::Node256::GetKeys(){
    std::vector<u_int8_t> node_keys;
    for(int i = 0; i < 256; ++i){
        if(children_[i] != nullptr){
            node_keys.push_back(i);
        }
    }
    return node_keys;
}


cas::Node* cas::Node256::Grow() {
  assert(false);
  return nullptr;
}


bool cas::Node256::IsFull() {
  return nr_children_ >= 256;
}


void cas::Node256::ReplaceBytePointer(uint8_t key_byte, cas::Node* child) {
  if (children_[key_byte] == nullptr) {
    assert(false);
  }
  children_[key_byte] = child;
}


void cas::Node256::ForEachChild(uint8_t low, uint8_t high,
                                  cas::ChildIt callback) {
  for (int i = high; i >= low; --i) {
    if (children_[i] != nullptr) {
      callback(static_cast<uint8_t>(i), *children_[i]);
    }
  }
}


size_t cas::Node256::SizeBytes() {
  return sizeof(cas::Node256) + (prefix_.capacity() * sizeof(uint8_t));
}


void cas::Node256::Dump() {
  std::cout << "type: Node256" << std::endl;
  cas::Node::Dump();
  std::cout << "children_: ";
  DumpAddresses(children_, 256);
  std::cout << std::endl;
  std::cout << std::endl;
}


int cas::Node256::NodeWidth() {
  return 256;
}
