#include "cas/node48.hpp"
#include "cas/node16.hpp"
#include "cas/node256.hpp"
#include <cassert>
#include <iostream>


cas::Node48::Node48(cas::NodeType type)
    : cas::Node(type) {
  memset(indexes_, kEmptyIndex, 256*sizeof(uint8_t));
  memset(children_, 0, 48*sizeof(uintptr_t));
}


void cas::Node48::Put(uint8_t key_byte, Node* child) {
  if (nr_children_ >= 48) {
    // TODO
    exit(-1);
  }
  int pos = 0;
  while (pos < 48 && children_[pos] != nullptr) {
    ++pos;
  }
  indexes_[key_byte] = pos;
  children_[pos] = child;
  ++nr_children_;
}


void cas::Node48::DeleteNode(uint8_t key_byte) {
  if (nr_children_ == 0) {
    // TODO
    std::cout << "** You cannot delete from empty Node48**" << std::endl;
    exit(-1);
  }
  //If key does not exists, it is error
  if(indexes_[key_byte] == kEmptyIndex){
    // TODO
    std::cout << "** You cannot delete non existing node from Node48**" << std::endl;
    exit(-1);
  }
  uint8_t pos = indexes_[key_byte];
  indexes_[key_byte] = kEmptyIndex;
  --nr_children_;
  std::memmove(children_+pos, children_+pos+1, (48-pos)*sizeof(uintptr_t));

  //all indexes that are greater that pos should be reduced by 1 since on position pos we removed child
  for (int i = 0; i < 256; i++) {
    if (indexes_[i] != kEmptyIndex && indexes_[i] > pos) {
      indexes_[i] = indexes_[i] - 1;
    }
  }

  //set rest to 0, but it is already regulated with the nur_children when we do a insert
//  memset(children_+nr_children_, 0, (48-nr_children_)*sizeof(uintptr_t));
}


cas::Node* cas::Node48::LocateChild(uint8_t key_byte) {
  uint8_t index = indexes_[key_byte];
  if (index == cas::kEmptyIndex) {
    return nullptr;
  } else {
    return children_[index];
  }
}


std::vector<uint8_t> cas::Node48::GetKeys(){
    std::vector<u_int8_t> node_keys;
    for(int i = 0; i < 256; ++i){
        if(indexes_[i] != cas::kEmptyIndex){
            node_keys.push_back(static_cast<uint8_t>(i));
        }
    }
    return node_keys;
}


cas::Node* cas::Node48::Grow() {
  cas::Node256* node256 = new cas::Node256(type_);
  node256->nr_children_ = 48;
  node256->separator_pos_ = separator_pos_;
  node256->prefix_ = std::move(prefix_);
  for (int i = 0; i < 256; ++i) {
    if (indexes_[i] != cas::kEmptyIndex) {
      node256->children_[i] = children_[indexes_[i]];
    }
  }
  return node256;
}


cas::Node* cas::Node48::Shrink() {
  assert(nr_children_ == 16);
  cas::Node16* node16 = new cas::Node16(type_);
  node16->nr_children_ = 16;
  node16->separator_pos_ = separator_pos_;
  node16->prefix_ = std::move(prefix_);
  int pos = 0;
  for (int i = 0; i < 256; ++i) {
    if (indexes_[i] != cas::kEmptyIndex) {
      node16->keys_[pos] = indexes_[i];
      node16->children_[pos] = children_[indexes_[i]];
      ++pos;
    }
  }
  return node16;
}


bool cas::Node48::IsFull() {
  return nr_children_ >= 48;
}


bool cas::Node48::IsUnderfilled() {
  return nr_children_ <= 16;
}


void cas::Node48::ReplaceBytePointer(uint8_t key_byte, cas::Node* child) {
  uint8_t pos = indexes_[key_byte];
  if (pos == cas::kEmptyIndex) {
    assert(false);
  }
  children_[pos] = child;
}


void cas::Node48::ForEachChild(uint8_t low, uint8_t high,
                                 cas::ChildIt callback) {
  for (int i = high; i >= low; --i) {
    if (indexes_[i] != cas::kEmptyIndex) {
      callback(static_cast<uint8_t>(i), *children_[indexes_[i]]);
    }
  }
}


size_t cas::Node48::SizeBytes() {
  return sizeof(cas::Node48) + (prefix_.capacity() * sizeof(uint8_t));
}


void cas::Node48::Dump() {
  std::cout << "type: Node48" << std::endl;
  cas::Node::Dump();
  std::cout << "keys_: ";
  DumpIndexes();
  std::cout << std::endl;
  std::cout << "children_: ";
  DumpAddresses(children_, 48);
  std::cout << std::endl;
  std::cout << std::endl;
}


void cas::Node48::DumpIndexes() {
  for (int i = 0; i < 256; ++i) {
    if (indexes_[i] != cas::kEmptyIndex) {
      printf("0x%02X(%d)", i, indexes_[i]);
      if (i < 256-1) {
        std::cout << " ";
      }
    }
  }
}


int cas::Node48::NodeWidth() {
  return 48;
}
