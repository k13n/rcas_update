#include "cas/cas_delete.hpp"
#include "cas/node0.hpp"
#include <algorithm>
#include <cassert>
#include <iostream>


template<class VType>
cas::CasDelete<VType>::CasDelete(cas::Node** root, const cas::BinaryKey& key)
  : root_(root)
  , key_(key)
{ }


template<class VType>
bool cas::CasDelete<VType>::Execute() {
  // look for th eleaf that needs to be deleted
  bool found = Traverse();
  if (!found) {
    return false;
  }
  auto* leaf = static_cast<cas::Node0*>(node_);

  // delete DID from leaf node
  DeleteDID(leaf->dids_, key_.did_);

  // Case 1: check if there are more DIDs contained in the leaf
  if (!leaf->dids_.empty()) {
    return true;
  }

  // Case *: check if the leaf to be deleted is the root node
  if (parent_ == nullptr) {
    delete *root_;
    *root_ = nullptr;
    return true;
  }

  // delete the current leaf from the parent and the leaf itself
  parent_->DeleteNode(parent_byte_);
  delete leaf;

  // Case 2: deleting the leaf does not require extensive restructuring
  if (parent_->nr_children_ > 1) {
    // the parent might become underfull (e.g., parent of type node48
    // should become a node16) and in this case we must shrink the parent
    if (parent_->IsUnderfilled()) {
      cas::Node* new_parent = parent_->Shrink();
      if (grand_parent_ == nullptr) {
        // the parent is the root node, hence we need to replace
        // the root node
        *root_ = new_parent;
      } else {
        // we replace the parent in the grand_parent_'s list of children
        // with the new_parent node
        grand_parent_->ReplaceBytePointer(grand_parent_byte_, new_parent);
      }
      delete parent_;
    }
    return true;
  }


  // Case 3: this is the difficult case; parent_ has only one child left
  // and these two nodes can be merged
  LazyDeletion();
  return true;
}


// this method merges node parent_ with its only remaining
// child node
template<class VType>
void cas::CasDelete<VType>::LazyDeletion() {
  auto key_bytes = parent_->GetKeys();
  assert(key_bytes.size() == 1);
  uint8_t next_byte = key_bytes[0];
  cas::Node* child = parent_->LocateChild(next_byte);

  // combine common prefixes
  std::vector<uint8_t> prefix;
  prefix.reserve(parent_->prefix_.size() + node_->prefix_.size());
  uint16_t separator_pos = 0;
  // copy common path prefixes
  std::copy(
      parent_->prefix_.begin(),
      parent_->prefix_.begin() + parent_->separator_pos_,
      std::back_inserter(prefix));
  if (parent_->type_ == cas::NodeType::Path) {
    prefix.push_back(next_byte);
  }
  std::copy(
      child->prefix_.begin(),
      child->prefix_.begin() + child->separator_pos_,
      std::back_inserter(prefix));
  separator_pos = static_cast<uint16_t>(prefix.size());
  // copy common value prefixes
  std::copy(
      parent_->prefix_.begin() + parent_->separator_pos_,
      parent_->prefix_.end(),
      std::back_inserter(prefix));
  if (parent_->type_ == cas::NodeType::Value) {
    prefix.push_back(next_byte);
  }
  std::copy(
      child->prefix_.begin() + child->separator_pos_,
      child->prefix_.end(),
      std::back_inserter(prefix));

  // update the child
  child->prefix_ = std::move(prefix);
  child->separator_pos_ = separator_pos;

  // delete the node parent_ and replace parent_ with child in
  // the grand_parent_'s list of children
  if (grand_parent_ == nullptr) {
    // parent_ was the root node, now child becomes the root node
    *root_ = child;
  } else {
    grand_parent_->ReplaceBytePointer(grand_parent_byte_, child);
  }
  delete parent_;
}



template<class VType>
bool cas::CasDelete<VType>::Traverse() {
  node_ = *root_;
  parent_ = nullptr;
  grand_parent_ = nullptr;
  parent_byte_ = 0;
  grand_parent_byte_ = 0;
  uint8_t next_byte = 0x00;

  size_t g_p = 0;
  size_t g_v = 0;
  while (node_ != nullptr) {
    size_t i_p = 0;
    size_t i_v = 0;
    // match the path prefix
    while (i_p < node_->PathPrefixSize() &&
           g_p < key_.path_.size() &&
           node_->prefix_[i_p] == key_.path_[g_p]) {
      ++i_p;
      ++g_p;
    }
    // match the value prefix
    while (i_v < node_->ValuePrefixSize() &&
           g_v < key_.value_.size() &&
           node_->prefix_[node_->separator_pos_ + i_v] == key_.value_[g_v]) {
      ++i_v;
      ++g_v;
    }

    // make sure we matched the whole node's prefix
    if (i_p < node_->PathPrefixSize() || i_v < node_->ValuePrefixSize()) {
      // we did not match the node's path/value prefix,
      // hence the key is not contained in the index
      return false;
    }

    // check if we reached a leaf
    if (g_p == key_.path_.size() && g_v == key_.value_.size()) {
      // we matched the whole key. make sure we are in a leaf
      // node and that we matched all of the node's p/v prefix
      assert(node_->IsLeaf());
      // make sure the DID is contained in the leaf
      for (const auto& did : static_cast<cas::Node0*>(node_)->dids_) {
        if (did == key_.did_) {
          return true;
        }
      }
      // the DID is not contained
      return false;
    }

    // we must traverse the node further
    if (node_->IsPathNode()) {
      assert(g_p < key_.path_.size());
      next_byte = key_.path_[g_p];
      ++g_p;
    } else if (node_->IsValueNode()) {
      assert(g_v < key_.value_.size());
      next_byte = key_.value_[g_v];
      ++g_v;
    } else {
      std::cerr << "reached a non-path / non-value node" << std::endl;
      return false;
    }

    grand_parent_byte_ = parent_byte_;
    parent_byte_ = next_byte;
    grand_parent_ = parent_;
    parent_ = node_;
    node_ = node_->LocateChild(next_byte);
  }

  return false;
}


template<class VType>
void cas::CasDelete<VType>::DeleteDID(std::vector<did_t>& v, cas::did_t did) {
  size_t i = 0;
  while (i < v.size()) {
    if (v[i] == did) {
      v.erase(v.begin() + i);
    } else {
      ++i;
    }
  }
}


// explicit instantiations to separate header from implementation
template class cas::CasDelete<cas::vint32_t>;
template class cas::CasDelete<cas::vint64_t>;
template class cas::CasDelete<cas::vstring_t>;
