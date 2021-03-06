#include "cas/locator.hpp"
#include <iostream>
#include <cassert>

//updating RCAS paper: alg 1
cas::Locator::Locator(cas::Node* root, cas::BinaryKey& bkey) :
    root_(root),
    bkey_(bkey)
  {}


void cas::Locator::Locate() {
  parent_ = nullptr;
  node_ = root_;

  size_t bkey_p_pos = 0;
  size_t bkey_v_pos = 0;
  uint8_t next_byte;

  matcher_.Match(node_, &bkey_, bkey_p_pos, bkey_v_pos);
  while (!matcher_.KeyFullyMatched() && !matcher_.HasMismatch()) {
    switch (node_->Type()) {
    case cas::NodeType::Path:
      next_byte  = bkey_.path_[matcher_.bkey_p_pos_];
      bkey_p_pos = matcher_.bkey_p_pos_ + 1;
      bkey_v_pos = matcher_.bkey_v_pos_;
      break;
    case cas::NodeType::Value:
      next_byte  = bkey_.value_[matcher_.bkey_v_pos_];
      bkey_p_pos = matcher_.bkey_p_pos_;
      bkey_v_pos = matcher_.bkey_v_pos_ + 1;
      break;
    case cas::NodeType::Leaf:
      assert(false);
      break;
    }
    cas::Node* child = node_->LocateChild(next_byte);
    if (child == nullptr) {
      return;
    } else {
      parent_ = node_;
      node_ = child;
      parent_byte_ = next_byte;
    }
    matcher_.Match(node_, &bkey_, bkey_p_pos, bkey_v_pos);
  }
}


void cas::Locator::Dump() {
  std::cout << "Locator:" << std::endl;
  std::cout << "bkey_:" << std::endl;
  bkey_.Dump();
  if (parent_ == nullptr) {
    std::cout << "parent_: nullptr" << std::endl;
  } else {
    std::cout << "parent_:" << std::endl;
    parent_->Dump();
  }
  printf("parent_byte_: 0x%02X\n", (unsigned char) parent_byte_);
  if (node_ == nullptr) {
    std::cout << "node_: nullptr" << std::endl;
  } else {
    std::cout << "node_:" << std::endl;
    node_->Dump();
  }
  std::cout << "Matcher:" << std::endl;
  matcher_.Dump();
  std::cout << std::endl;
}
