#include "cas/bulk_load.hpp"
#include "cas/node0.hpp"
#include "cas/node4.hpp"
#include "cas/node16.hpp"
#include "cas/node48.hpp"
#include "cas/node256.hpp"
#include <iostream>


cas::BulkLoad::BulkLoad(std::deque<cas::BinaryKey>& keys,
      cas::NodeType root_split)
  : keys_(keys),
  root_split_(root_split) {}

cas::Node* cas::BulkLoad::Execute() {
  if (keys_.empty()) {
    return nullptr;
  }
  
  std::vector<size_t> indexes(keys_.size(), 0);
  for (size_t i = 0; i < keys_.size(); ++i) {
    indexes[i] = i;
  }
  return Construct(indexes, 0, 0, root_split_);
}


cas::Node* cas::BulkLoad::Construct(
    const std::vector<size_t>& indexes,
    size_t dp, size_t dv, cas::NodeType split_type) {
  auto& some_key = keys_[indexes[0]];
  size_t dp_new = DiscriminativeByte(indexes, cas::NodeType::Path, dp);
  size_t dv_new = DiscriminativeByte(indexes, cas::NodeType::Value, dv);

  if (dp_new == DoesNotExist && dv_new == DoesNotExist) {
    // we reached a leaf node
    cas::Node0* leaf = new Node0();
    leaf->nr_keys_ = indexes.size();
    BuildPrefix(leaf, some_key, dp, dv, dp_new, dv_new);
    leaf->dids_.reserve(indexes.size());
    for (size_t index : indexes) {
      cas::BinaryKey& key = keys_[index];
      leaf->dids_.push_back(key.did_);
    }
    return leaf;
  }

  if (split_type == cas::NodeType::Path && dp_new == DoesNotExist) {
    split_type = cas::NodeType::Value;
  } else if (split_type == cas::NodeType::Value && dv_new == DoesNotExist) {
    split_type = cas::NodeType::Path;
  }

  auto partitions = Partition(indexes, split_type,
      split_type == cas::NodeType::Path ? dp_new : dv_new);

  cas::Node* node;
  if (partitions.size() <= 4) {
    node = new Node4(split_type);
  } else if (partitions.size() <= 16) {
    node = new Node16(split_type);
  } else if (partitions.size() <= 48) {
    node = new Node48(split_type);
  } else {
    node = new Node256(split_type);
  }
  BuildPrefix(node, some_key, dp, dv, dp_new, dv_new);

  for (const auto& it : partitions) {
    cas::Node* child;
    if (split_type == cas::NodeType::Path) {
      child = Construct(it.second, dp_new+1, dv_new, cas::NodeType::Value);

    } else {
      child = Construct(it.second, dp_new, dv_new+1, cas::NodeType::Path);
    }
    node->Put(it.first, child);
  }

    node->ForEachChild([&](uint8_t, cas::Node& child) -> bool {
        node->nr_keys_ += child.nr_keys_;
        return true;
    });
  return node;
}


void cas::BulkLoad::BuildPrefix(
    cas::Node* node,
    cas::BinaryKey& key,
    size_t dp, size_t dv,
    size_t dp_new, size_t dv_new) {
  const auto& path  = key.Get(cas::NodeType::Path);
  const auto& value = key.Get(cas::NodeType::Value);
  if (dp     == DoesNotExist) { dp     = path.size();  }
  if (dp_new == DoesNotExist) { dp_new = path.size();  }
  if (dv     == DoesNotExist) { dv     = value.size(); }
  if (dv_new == DoesNotExist) { dv_new = value.size(); }
  node->prefix_.reserve((dp_new - dp) + (dv_new - dv));
  std::copy(path.begin() + dp, path.begin() + dp_new,
      std::back_inserter(node->prefix_));
  node->separator_pos_ = node->prefix_.size();
  std::copy(value.begin() + dv, value.begin() + dv_new,
      std::back_inserter(node->prefix_));
}


std::map<uint8_t, std::vector<size_t>> cas::BulkLoad::Partition(
    const std::vector<size_t>& indexes,
    cas::NodeType attribute, size_t disc_byte) {
  std::map<uint8_t, std::vector<size_t>> partitions;
  for (size_t index : indexes) {
    cas::BinaryKey& key = keys_[index];
    uint8_t byte = key.Get(attribute)[disc_byte];
    partitions[byte].push_back(index);
  }
  return partitions;
}


size_t cas::BulkLoad::DiscriminativeByte(
    const std::vector<size_t>& indexes,
    cas::NodeType attribute, size_t lower_bound) {
  const auto& some_string = keys_[indexes[0]].Get(attribute);
  while (lower_bound < some_string.size()) {
    uint8_t byte = some_string[lower_bound];
    for (size_t index : indexes) {
      cas::BinaryKey& key = keys_[index];
      const auto& string = key.Get(attribute);
      if (string[lower_bound] != byte) {
        return lower_bound;
      }
    }
    ++lower_bound;
  }
  return DoesNotExist;
}
