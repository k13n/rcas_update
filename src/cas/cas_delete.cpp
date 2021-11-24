#include "cas/cas_delete.hpp"
#include "cas/node_type.hpp"
#include "cas/bulk_load.hpp"
#include "cas/node0.hpp"
#include "cas/utils.hpp"
#include <algorithm>
#include <cassert>
#include <deque>
#include <iostream>


template<class VType>
cas::CasDelete<VType>::CasDelete(
        cas::Node** root_main,
        cas::Node** root_auxiliary,
        const cas::BinaryKey& key,
        cas::UpdateType deletion_method)
  : root_main_(root_main)
  , root_auxiliary_(root_auxiliary)
  , key_(key)
  , deletion_method_(deletion_method)
{ }


template<class VType>
bool cas::CasDelete<VType>::Execute() {
  bool success = Execute(root_main_);
  if (!success && *root_auxiliary_ != nullptr) {
    success = Execute(root_auxiliary_);
  }
  return success;
}


template<class VType>
bool cas::CasDelete<VType>::Execute(cas::Node** root) {
  // look for th eleaf that needs to be deleted
  bool found = Traverse(root);
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
    delete *root;
    *root = nullptr;
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
        *root = new_parent;
      } else {
        // we replace the parent in the grand_parent_'s list of children
        // with the new_parent node
        grand_parent_->ReplaceBytePointer(grand_parent_byte_, new_parent);
      }
      delete parent_;
      parent_ = new_parent;
    }
    PerformPrefixPullup();
    return true;
  }

  // Case 3: this is the difficult case; parent_ has only one child left
  // and these two nodes can be merged
  switch (deletion_method_) {
    case cas::UpdateType::LazyFast:
      LazyDeletion(root);
      break;
    case cas::UpdateType::StrictSlow:
      StrictDeletion(root);
      break;
    default:
      break;
  }
  return true;
}


template<class VType>
void cas::CasDelete<VType>::PerformPrefixPullup() {
  // there can only be a common prefix in the alternate dimension
  cas::NodeType dimension = Alternate(parent_->type_);

  // compute common prefix in the alternate dimension
  bool first = true;
  uint8_t* ref_prefix = nullptr;
  size_t ref_prefix_len = 0;
  size_t dsc = 0;
  parent_->ForEachChild([&](uint8_t /* byte */, cas::Node& child) -> bool {
    uint8_t* prefix = child.Prefix(dimension);
    size_t prefix_len = child.PrefixLen(dimension);
    if (first) {
      first = false;
      ref_prefix = prefix;
      ref_prefix_len = prefix_len;
      dsc = ref_prefix_len;
    } else {
      size_t i = 0;
      while (i < dsc && i < prefix_len && ref_prefix[i] == prefix[i]) {
        ++i;
      }
      dsc = i;
    }
    return true;
  });

  // check if there is no common prefix
  if (dsc == 0) {
    return;
  }

  // add common prefix to the parent node's prefix
  std::vector<uint8_t> parent_prefix;
  std::copy(
      parent_->prefix_.begin(),
      parent_->prefix_.begin() + parent_->separator_pos_,
      std::back_inserter(parent_prefix));
  if (dimension == cas::NodeType::Path) {
    std::copy(ref_prefix, ref_prefix + dsc, std::back_inserter(parent_prefix));
  }
  size_t parent_separator_pos = parent_prefix.size();
  std::copy(
      parent_->prefix_.begin() + parent_->separator_pos_,
      parent_->prefix_.end(),
      std::back_inserter(parent_prefix));
  if (dimension == cas::NodeType::Value) {
    std::copy(ref_prefix, ref_prefix + dsc, std::back_inserter(parent_prefix));
  }
  parent_->prefix_ = parent_prefix;
  parent_->separator_pos_ = parent_separator_pos;

  // remove common prefix from the children
  parent_->ForEachChild([&](uint8_t /* byte */, cas::Node& child) -> bool {
    std::vector<uint8_t> prefix;
    auto path_begin = child.prefix_.begin();
    if (dimension == cas::NodeType::Path) {
      path_begin = path_begin + dsc;
    }
    std::copy(
        path_begin,
        child.prefix_.begin() + child.separator_pos_,
        std::back_inserter(prefix));
    size_t separator_pos = prefix.size();
    auto value_begin = child.prefix_.begin() + child.separator_pos_;
    if (dimension == cas::NodeType::Value) {
      value_begin = value_begin + dsc;
    }
    std::copy(
        value_begin,
        child.prefix_.end(),
        std::back_inserter(prefix));
    child.prefix_ = prefix;
    child.separator_pos_ = separator_pos;
    return true;
  });
}


template<class VType>
cas::NodeType cas::CasDelete<VType>::Alternate(cas::NodeType type) {
  switch (type) {
    case cas::NodeType::Path:  return cas::NodeType::Value;
    case cas::NodeType::Value: return cas::NodeType::Path;
    case cas::NodeType::Leaf:
      throw std::runtime_error{"invalid node type"};
  }
  return cas::NodeType::Leaf;
}


// this method merges node parent_ with its only remaining
// child node in a lazy way
template<class VType>
void cas::CasDelete<VType>::LazyDeletion(cas::Node** root) {
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
    *root = child;
  } else {
    grand_parent_->ReplaceBytePointer(grand_parent_byte_, child);
  }
  delete parent_;
}


// this method merges node parent_ with its only remaining
// child node in a lazy way
template<class VType>
void cas::CasDelete<VType>::StrictDeletion(cas::Node** root) {
  struct PartialKey {
    cas::Node* node_;
    std::vector<uint8_t> path_;
    std::vector<uint8_t> value_;

    void Dump() const {
      std::cout << "Path: ";
      cas::Utils::DumpHexValues(path_);
      std::cout << "Value: ";
      cas::Utils::DumpHexValues(value_);
      std::cout << "\n";
    }
  };
  std::deque<PartialKey> partial_key_stack;
  std::deque<cas::BinaryKey> keys;

  PartialKey root_pkey;
  root_pkey.node_ = parent_;
  std::copy(
      parent_->prefix_.begin(),
      parent_->prefix_.begin() + parent_->separator_pos_,
      std::back_inserter(root_pkey.path_));
  std::copy(
      parent_->prefix_.begin() + parent_->separator_pos_,
      parent_->prefix_.end(),
      std::back_inserter(root_pkey.value_));
  partial_key_stack.push_back(root_pkey);

  // collect binary keys by recursively traversing the index
  while (!partial_key_stack.empty()) {
    auto pkey = partial_key_stack.back();
    partial_key_stack.pop_back();
    pkey.node_->ForEachChild([&](uint8_t next_byte, cas::Node& child) -> bool {
      auto pkey_child = pkey;
      pkey_child.node_ = &child;
      if (pkey.node_->IsPathNode()) {
        pkey_child.path_.push_back(next_byte);
      } else {
        pkey_child.value_.push_back(next_byte);
      }
      std::copy(
          child.prefix_.begin(),
          child.prefix_.begin() + child.separator_pos_,
          std::back_inserter(pkey_child.path_));
      std::copy(
          child.prefix_.begin() + child.separator_pos_,
          child.prefix_.end(),
          std::back_inserter(pkey_child.value_));
      partial_key_stack.push_back(pkey_child);
      return true;
    });
    if (pkey.node_->IsLeaf()) {
      cas::BinaryKey bkey;
      auto* leaf = static_cast<cas::Node0*>(pkey.node_);
      for (auto did : leaf->dids_) {
        bkey.path_ = std::move(pkey.path_);
        bkey.value_ = std::move(pkey.value_);
        bkey.did_ = did;
        keys.push_back(bkey);
      }
    }
  }

  // determine the dimension of the new subtree's root node
  cas::NodeType root_dimension;
  if (grand_parent_ == nullptr) {
    root_dimension = parent_->type_;
  } else {
    root_dimension = grand_parent_->IsPathNode()
      ? cas::NodeType::Value
      : cas::NodeType::Path;
  }
  cas::BulkLoad bulk_loader{keys, root_dimension};
  cas::Node* new_parent = bulk_loader.Execute();

  // install new_parent in the tree
  if (grand_parent_ == nullptr) {
    // parent_ is the root node
    *root = new_parent;
  } else {
    // replace parent_ with new_parent in grand_parent_'s
    // list of children
    grand_parent_->ReplaceBytePointer(grand_parent_byte_, new_parent);
  }

  // delete old subtree recursively
  std::function<void(cas::Node*)> DeleteSubtree = [&DeleteSubtree](cas::Node *node) {
    if (node == nullptr) {
      return;
    }
    node->ForEachChild([&](uint8_t, cas::Node& child) -> bool {
      DeleteSubtree(&child);
      return true;
    });
    delete node;
  };
  DeleteSubtree(parent_);
}


template<class VType>
bool cas::CasDelete<VType>::Traverse(cas::Node** root) {
  node_ = *root;
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
