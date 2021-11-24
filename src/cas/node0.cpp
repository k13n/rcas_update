#include "cas/node0.hpp"
#include <algorithm>
#include <cassert>
#include <iostream>


cas::Node0::Node0() : cas::Node(cas::NodeType::Leaf) {}


cas::Node0::Node0(const cas::BinaryKey& bkey, size_t path_pos, size_t value_pos)
    : cas::Node(cas::NodeType::Leaf) {
  size_t size = (bkey.path_.size() - path_pos) +
                (bkey.value_.size() - value_pos);
  prefix_.reserve(size);
  // fill prefix_ with the remaining value bytes
  std::copy(bkey.path_.begin() + path_pos, bkey.path_.end(),
      std::back_inserter(prefix_));
  // fill prefix_ with the remaining path bytes
  separator_pos_ = prefix_.size();
  std::copy(bkey.value_.begin() + value_pos, bkey.value_.end(),
      std::back_inserter(prefix_));
  // add the DID
  dids_.push_back(bkey.did_);
}


cas::Node0::Node0(const InterleavedKey& ikey, size_t pos)
    : cas::Node(cas::NodeType::Leaf) {
  assert(ikey.bytes_.size() >= pos);
  prefix_.reserve(ikey.bytes_.size() - pos);

  for (size_t i = pos; i < ikey.bytes_.size(); ++i) {
    if (ikey.bytes_[i].type_ == cas::NodeType::Path) {
      prefix_.push_back(ikey.bytes_[i].byte_);
    }
  }
  separator_pos_ = prefix_.size();
  for (size_t i = pos; i < ikey.bytes_.size(); ++i) {
    if (ikey.bytes_[i].type_ == cas::NodeType::Value) {
      prefix_.push_back(ikey.bytes_[i].byte_);
    }
  }

  dids_.push_back(ikey.did_);
}


std::vector<uint8_t> cas::Node0::GetKeys(){
    return std::vector<u_int8_t>();
}

void cas::Node0::Put(uint8_t /*key_byte*/, cas::Node* /*child*/) {
  // TODO
  exit(-1);
}

void cas::Node0::DeleteNode(uint8_t key_byte) {
    // TODO
    exit(-1);
}

cas::Node* cas::Node0::LocateChild(uint8_t /*key_byte*/) {
  return nullptr;
}


bool cas::Node0::ContainsDid(cas::did_t did) {
  return std::find(dids_.begin(), dids_.end(), did) != dids_.end();
}


void cas::Node0::ReplaceBytePointer(uint8_t /*byte*/, cas::Node* /*child*/) {
  assert(false);
}


void cas::Node0::ForEachChild(uint8_t /*low*/, uint8_t /*high*/,
                                cas::ChildIt /*callback*/) {
  // has no children
}


size_t cas::Node0::SizeBytes() {
  return sizeof(cas::Node0) +
    (prefix_.capacity() * sizeof(uint8_t)) +
    (dids_.capacity() * sizeof(cas::did_t));
}


void cas::Node0::Dump() {
  std::cout << "type: Node0" << std::endl;
  cas::Node::Dump();
  std::cout << "DIDs: ";
  for (const auto& did : dids_) {
    std::cout << did << " ";
  }
  std::cout << std::endl;
  std::cout << std::endl;
}


int cas::Node0::NodeWidth() {
  return 0;
}
