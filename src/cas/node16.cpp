#include "cas/node16.hpp"
#include "cas/node48.hpp"
#include "cas/node4.hpp"
#include <cassert>
#include <iostream>


cas::Node16::Node16(cas::NodeType type)
    : cas::Node(type) {
  memset(keys_, 0, 16*sizeof(uint8_t));
  memset(children_, 0, 16*sizeof(uintptr_t));
}


void cas::Node16::Put(uint8_t key_byte, Node* child) {
  if (nr_children_ >= 16) {
    // TODO
    exit(-1);
  }
  int pos = 0;
  while (pos < nr_children_ && key_byte > keys_[pos]) {
    ++pos;
  }
  std::memmove(keys_+pos+1, keys_+pos, (nr_children_-pos)*sizeof(uint8_t));
  std::memmove(children_+pos+1, children_+pos, (nr_children_-pos)*sizeof(uintptr_t));
  keys_[pos] = key_byte;
  children_[pos] = child;
  ++nr_children_;
}


void cas::Node16::DeleteNode(uint8_t key_byte) {
  if (nr_children_ == 0) {
    // TODO
    std::cout << "** You cannot delete from empty Node16**" << std::endl;
    exit(-1);
  }
  int pos = 0;
  while (pos < nr_children_ && key_byte != keys_[pos]) {
    ++pos;
  }
  //If key does not exists, it is error
  if (pos == nr_children_) {
    // TODO
    std::cout << "** You cannot delete non existing node from Node16**" << std::endl;
    exit(-1);
  }
  --nr_children_;
  std::memmove(keys_+pos, keys_+pos+1, (nr_children_-pos)*sizeof(uint8_t));
  std::memmove(children_+pos, children_+pos+1, (nr_children_-pos)*sizeof(uintptr_t));

  //set rest to 0, but it is already regulated with the nur_children when we do a insert
  memset(keys_+nr_children_, 0, (16-nr_children_)*sizeof(uint8_t));
  memset(children_+nr_children_, 0, (16-nr_children_)*sizeof(uintptr_t));
}


cas::Node* cas::Node16::LocateChild(uint8_t key_byte) {
  for (int i = 0; i < nr_children_; ++i) {
    if (key_byte == keys_[i]) {
      return children_[i];
    }
  }
  return nullptr;
}


std::vector<uint8_t> cas::Node16::GetKeys(){
    std::vector<u_int8_t> node_keys;
    for(int i = 0; i < nr_children_; ++i){
        node_keys.push_back(keys_[i]);
    }
    return node_keys;
}


cas::Node* cas::Node16::Grow() {
  cas::Node48* node48 = new cas::Node48(type_);
  node48->nr_children_ = 16;
  node48->separator_pos_ = separator_pos_;
  node48->prefix_ = std::move(prefix_);
  for (int i = 0; i < nr_children_; ++i) {
    node48->indexes_[keys_[i]] = i;
  }
  std::memcpy(node48->children_, children_, 16*sizeof(uintptr_t));
  return node48;
}


cas::Node* cas::Node16::Shrink() {
  assert(nr_children_ == 4);
  cas::Node4* node4 = new cas::Node4(type_);
  node4->nr_children_ = 4;
  node4->separator_pos_ = separator_pos_;
  node4->prefix_ = std::move(prefix_);
  for (int i = 0; i < nr_children_; ++i) {
    node4->keys_[i] = keys_[i];
  }
  std::memcpy(node4->children_, children_, 4*sizeof(uintptr_t));
  return node4;
}


void cas::Node16::ReplaceBytePointer(uint8_t key_byte, cas::Node* child) {
  for (int i = 0; i < nr_children_; ++i) {
    if (keys_[i] == key_byte) {
      children_[i] = child;
      return;
    }
  }
  assert(false);
}


void cas::Node16::ForEachChild(uint8_t low, uint8_t high,
                                 cas::ChildIt callback) {
  for (int i = nr_children_-1; i >= 0 && keys_[i] >= low; --i) {
    if (keys_[i] <= high) {
      callback(keys_[i], *children_[i]);
    }
  }
}


bool cas::Node16::IsFull() {
  return nr_children_ >= 16;
}


bool cas::Node16::IsUnderfilled() {
  return nr_children_ <= 4;
}



size_t cas::Node16::SizeBytes() {
  return sizeof(cas::Node16) + (prefix_.capacity() * sizeof(uint8_t));
}


void cas::Node16::Dump() {
  std::cout << "type: Node16" << std::endl;
  cas::Node::Dump();
  std::cout << "keys_: ";
  DumpBuffer(keys_, 16);
  std::cout << std::endl;
  std::cout << "children_: ";
  DumpAddresses(children_, 16);
  std::cout << std::endl;
  std::cout << std::endl;
}


int cas::Node16::NodeWidth() {
  return 16;
}
