#include "cas/cas.hpp"
#include "cas/key_encoder.hpp"
#include "cas/node0.hpp"
#include "cas/node4.hpp"
#include "cas/prefix_matcher.hpp"
#include "cas/interleaved_key.hpp"
#include "cas/interleaver.hpp"
#include "cas/cas_delete.hpp"
#include "cas/cas_insert.hpp"
#include "cas/query.hpp"
#include "cas/search_key.hpp"
#include "cas/key_decoder.hpp"
#include "cas/utils.hpp"
#include "cas/bulk_load.hpp"
#include <iostream>
#include <cassert>
#include <chrono>


template<class VType>
cas::Cas<VType>::Cas(cas::IndexType type, const std::vector<std::string>& query_path)
  : cas::Index<VType>(query_path)
  , index_type_(type)
  , surrogate_(0, 0)
  , use_surrogate_(false)
{ }


template<class VType>
cas::Cas<VType>::Cas(cas::IndexType type, const std::vector<std::string>& query_path,
    size_t s_max_depth, size_t s_bytes_per_label)
  : cas::Index<VType>(query_path)
  , index_type_(type)
  , surrogate_(s_max_depth, s_bytes_per_label)
  , use_surrogate_(true)
{ }


template<class VType>
cas::Cas<VType>::~Cas() {
  DeleteNodesRecursively(root_);
  DeleteNodesRecursively(auxiliary_index_);
}


template<class VType>
void cas::Cas<VType>::DeleteNodesRecursively(cas::Node *node) {
  if (node == nullptr) {
    return;
  }
  node->ForEachChild([&](uint8_t, cas::Node& child) -> bool {
    DeleteNodesRecursively(&child);
    return true;
  });
  delete node;
}


template<class VType>
cas::QueryStats cas::Cas<VType>::Insert(
    cas::Key<VType>& key,
    cas::InsertType insertTypeMain,
    cas::InsertType insertTypeAux,
    cas::InsertTarget insert_target
  ) {
  // TODO

  cas::Node* node = root_;

  // Create a new root if the index is empty
  if(node == nullptr){
    std::deque<cas::Key<VType>> keys;
    keys.push_front(key);
    cas::Cas<VType>::BulkLoad(keys);
    std::cout << "\n"<<"root_ is empty, new index is created"<<"\n";

    return cas::QueryStats();
  }

  cas::KeyEncoder<VType> encoder;
  cas::SearchKey<VType> skey;

  //recreate the original path that was separated into labels in CasImporter::Insert()
  std::string joined_path_;
  for (auto& label : key.path_) {
    joined_path_.append("/");
    joined_path_.append(label.data());
  }

  skey.path_ = { joined_path_ };
  skey.low_  = key.value_;
  skey.high_ = key.value_;

  cas::BinarySK bkey = encoder.EncodeInsertKey(skey);
  cas::InsertionHelper pm;

  // We do this to add trailing null byte at the end of the path
  std::vector<uint8_t> mod_path_  = std::vector<uint8_t>(bkey.path_.bytes_.size() + 1);
  std::memcpy(&mod_path_[0], &bkey.path_.bytes_[0], bkey.path_.bytes_.size());
  mod_path_[bkey.path_.bytes_.size()] = cas::kNullByte;
  bkey.path_.bytes_ = mod_path_;



  switch (insert_target) {
    case cas::InsertTarget::MainOnly: {
      // main index only
      cas::CasInsert<VType> casInsert_main(root_, bkey, pm, key.did_, auxiliary_index_, false);
      casInsert_main.Execute(root_, insertTypeMain, auxiliary_index_);
      return casInsert_main.Stats();
    }
    case cas::InsertTarget::AuxiliaryOnly: {
      // auxiliary_index_ only
      cas::CasInsert<VType> casInsert_auxiliary(auxiliary_index_, bkey, pm, key.did_, root_, false);
      casInsert_auxiliary.Execute(auxiliary_index_, insertTypeAux, root_);
      return casInsert_auxiliary.Stats();
    }
    case cas::InsertTarget::MainAuxiliary: {
      // auxiliary_index_ only
      cas::CasInsert<VType> casInsert_main(root_, bkey, pm, key.did_, auxiliary_index_, true);
      cas::CasInsert<VType> casInsert_auxiliary(auxiliary_index_, bkey, pm, key.did_, root_, false);
      if (casInsert_main.Execute(root_, insertTypeMain, auxiliary_index_) == false){
        casInsert_auxiliary.Execute(auxiliary_index_, insertTypeAux, root_);
      }
      QueryStats overall_stats;
      overall_stats.runtime_main_mus_ = casInsert_main.Stats().runtime_mus_;
      overall_stats.runtime_aux_mus_ = casInsert_auxiliary.Stats().runtime_mus_;
      overall_stats.runtime_mus_ = overall_stats.runtime_main_mus_ + overall_stats.runtime_aux_mus_;
      return overall_stats;
    }
    default:
      throw std::runtime_error{"option does not exist"};
  }
}


template<class VType>
void cas::Cas<VType>::Insert(const cas::BinaryKey& /*bkey*/) {
  // TODO
}


template<class VType>
void cas::Cas<VType>::Delete(const cas::BinaryKey& bkey) {
  cas::CasDelete<VType> deleter{&root_, bkey};
  deleter.Execute();
}


template<class VType>
uint64_t cas::Cas<VType>::BulkLoad(std::deque<cas::Key<VType>>& keys) {

  const auto& t_start = std::chrono::high_resolution_clock::now();

  cas::KeyEncoder<VType> encoder;
  std::deque<cas::BinaryKey> bkeys;
  for (const auto& key : keys) {
    if (use_surrogate_) {
      bkeys.push_back(encoder.Encode(key, surrogate_));
    } else {
      bkeys.push_back(encoder.Encode(key));
    }
  }
  // releasing memory for old keys
  keys.clear();
  keys.shrink_to_fit();

  BulkLoad(bkeys);

  const auto& t_end = std::chrono::high_resolution_clock::now();
  return std::chrono::duration_cast<std::chrono::microseconds>(t_end-t_start).count();
}


template<class VType>
uint64_t cas::Cas<VType>::BulkLoad(std::deque<cas::BinaryKey>& keys, cas::NodeType nodeType) {
  assert(index_type_ == cas::IndexType::TwoDimensional);
  cas::BulkLoad load(keys, nodeType);
  root_ = load.Execute();
  nr_keys_ = keys.size();
    return 0;
}




template<class VType>
const cas::QueryStats cas::Cas<VType>::Query(
    cas::SearchKey<VType>& key,
    cas::BinaryKeyEmitter emitter) {
  cas::KeyEncoder<VType> encoder;
  if (use_surrogate_) {
    cas::BinarySK bkey = encoder.Encode(key, surrogate_);
    cas::SurrogatePathMatcher pm(surrogate_);
    cas::Query<VType> query(root_, bkey, pm, emitter);
    query.Execute();
    return query.Stats();
  } else {
    cas::BinarySK bkey = encoder.Encode(key);
    cas::PathMatcher pm;
    cas::Query<VType> query(root_, bkey, pm, emitter);

    //Use of Auxiliary index case
//  if(auxiliary_index_ != nullptr) std::cout<<"Number of keys in the auxiliary index: " << auxiliary_index_->nr_keys_ << std::endl;
    query.setAuxiliaryIndex(auxiliary_index_);

    query.Execute();
    return query.Stats();
  }
}


template<class VType>
const cas::QueryStats cas::Cas<VType>::Query(
    cas::SearchKey<VType>& key,
    cas::DidEmitter emitter) {
  return Query(key, [&](
        const std::vector<uint8_t>& /*buffer_path*/,
        const std::vector<uint8_t>& /*buffer_value*/,
        cas::did_t did) -> void {
    emitter(did);
  });
}


template<class VType>
const cas::QueryStats cas::Cas<VType>::Query(
    cas::SearchKey<VType>& key,
    cas::Emitter<VType> emitter) {
  cas::KeyDecoder<VType> decoder;
  return Query(key, [&](
        const std::vector<uint8_t>& buffer_path,
        const std::vector<uint8_t>& buffer_value,
        cas::did_t did) -> void {
    if (use_surrogate_) {
      emitter(decoder.Decode(surrogate_, buffer_path, buffer_value, did));
    } else {
      emitter(decoder.Decode(buffer_path, buffer_value, did));
    }
  });
}


template<class VType>
const cas::QueryStats cas::Cas<VType>::QueryRuntime(
    cas::SearchKey<VType>& key) {
  return Query(key, [&](
        const std::vector<uint8_t>& /*buffer_path*/,
        const std::vector<uint8_t>& /*buffer_value*/,
        cas::did_t /*did*/) -> void {
  });
}


template<class VType>
size_t cas::Cas<VType>::Size() {
  return nr_keys_;
}


template<class VType>
const cas::IndexStats cas::Cas<VType>::Stats() {
  cas::IndexStats stats;
  if (root_ != nullptr) {
    root_->CollectStats(stats, 0);
  }
  stats.nr_keys_ = nr_keys_;
  return stats;
}


template<class VType>
void cas::Cas<VType>::setAuxiliaryIndex(Node *index) {
    auxiliary_index_ = index;
}


template<class VType>
cas::Node* cas::Cas<VType>::getAuxiliaryIndex() {
    return auxiliary_index_;
}

template<class VType>
void cas::Cas<VType>::mergeMainAndAuxiliaryIndex(cas::MergeMethod merge_method){
  if (auxiliary_index_ == nullptr) {
    return;
  }
  cas::BinarySK bkey;
  cas::InsertionHelper pm;
  did_t did_ = 0;
  std::stack<Node*> traversed_nodes_sec;
  cas::CasInsert<VType> casInsert_auxiliary(
      auxiliary_index_,
      bkey,
      pm,
      did_,
      root_,
      false,
      merge_method);
  casInsert_auxiliary.mergeIndexes(
      auxiliary_index_,
      root_,
      nullptr,
      nullptr,
      0x00,
      0x00,
      cas::NodeType::Path,
      cas::NodeType::Path,
      traversed_nodes_sec);
  auxiliary_index_ = nullptr;
  root_ = casInsert_auxiliary.getSecondIndex();
}


template<class VType>
void cas::Cas<VType>::Describe() {
  switch (index_type_) {
  case cas::IndexType::TwoDimensional:
    std::cout << "Cas";
    break;
  case cas::IndexType::PathValue:
    std::cout << "CasPV";
    break;
  case cas::IndexType::ValuePath:
    std::cout << "CasVP";
    break;
  case cas::IndexType::ByteInterleaving:
    std::cout << "CasBI";
    break;
  case cas::IndexType::LevelInterleaving:
    std::cout << "CasLI";
    break;
  case cas::IndexType::ZOrder:
    std::cout << "CasZO";
    break;
  case cas::IndexType::Xml:
  case cas::IndexType::Seq:
    assert(false);
  }
  std::cout << "<" << cas::Utils::TypeToString<VType>() << ">" << std::endl;
  std::cout << "Query Path:   ";
  for (auto& label : cas::Index<VType>::query_path_) {
    std::cout << "/" << label;
  }
  std::cout << std::endl;
  std::cout << "Size (keys):  " << nr_keys_ << std::endl;
  cas::IndexStats stats;
  if (root_ != nullptr) {
    root_->CollectStats(stats, 0);
  }
  std::cout << "Size (bytes): " << stats.size_bytes_ << std::endl;
  double bpk = nr_keys_ == 0 ? 0 :
      stats.size_bytes_ / static_cast<double>(nr_keys_);
  size_t internal_nodes = stats.nr_nodes_ - stats.nr_node0_;
  std::cout << "Bytes/Key:    " << bpk << std::endl;
  std::cout << "Total Nodes:  " << stats.nr_nodes_ << std::endl;
  std::cout << "Inner Nodes:  " << internal_nodes << std::endl;
  std::cout << "Path Nodes:   " << stats.nr_path_nodes_ << std::endl;
  std::cout << "Value Nodes:  " << stats.nr_value_nodes_ << std::endl;
  std::cout << "Node0:        " << stats.nr_node0_ << std::endl;
  std::cout << "P-Node4:      " << stats.nr_p_node4_ << std::endl;
  std::cout << "P-Node16:     " << stats.nr_p_node16_ << std::endl;
  std::cout << "P-Node48:     " << stats.nr_p_node48_ << std::endl;
  std::cout << "P-Node256:    " << stats.nr_p_node256_ << std::endl;
  std::cout << "V-Node4:      " << stats.nr_v_node4_ << std::endl;
  std::cout << "V-Node16:     " << stats.nr_v_node16_ << std::endl;
  std::cout << "V-Node48:     " << stats.nr_v_node48_ << std::endl;
  std::cout << "V-Node256:    " << stats.nr_v_node256_ << std::endl;
  std::cout << "PP Steps:     " << stats.pp_steps << std::endl;
  std::cout << "VV Steps:     " << stats.vv_steps << std::endl;
  std::cout << "PV Steps:     " << stats.pv_steps << std::endl;
  std::cout << "VP Steps:     " << stats.vp_steps << std::endl;
  std::cout << "Max Depth:    " << stats.max_depth_ << std::endl;
  std::cout << "Surrogate:    " << (use_surrogate_ ? "yes" : "no") << std::endl;
  std::cout << "S. MaxDepth:  " << surrogate_.max_depth_ << std::endl;
  std::cout << "S. BytesPerLabel: " << surrogate_.bytes_per_label_ << std::endl;

  double avg_fanout = 0;
  if (internal_nodes > 0) {
    avg_fanout = stats.nr_nodes_ / static_cast<double>(internal_nodes);
  }
  std::cout << "Avg Fanout:   " << avg_fanout << std::endl;

  size_t step_diff = stats.pv_steps + stats.vp_steps;
  size_t step_all  = stats.pp_steps + stats.vv_steps + step_diff;
  double il_ratio = 0;
  if (step_all > 0) {
    il_ratio = step_diff / static_cast<double>(step_all);
  }
  std::cout << "IL Ratio:     " << il_ratio << std::endl;

  std::cout << std::endl;
}


template<class VType>
void cas::Cas<VType>::Dump() {
  if (root_ != nullptr) {
    root_->DumpRecursive();
  }
}


template<class VType>
void cas::Cas<VType>::DumpConcise() {
  if (root_ != nullptr) {
    root_->DumpConcise(0x00, 0);
    std::cout<<std::endl;
    if(auxiliary_index_ != nullptr) auxiliary_index_->DumpConcise(0x00, 0);
  }
}


template<class VType>
void cas::Cas<VType>::DumpLatex(const std::string& message) {
  if (root_ != nullptr) {
    std::cout << "\\begin{forest}" << std::endl;
    std::cout << "  casindex," << std::endl;
    DumpLatexRoot();
    switch (root_->type_) {
    case cas::NodeType::Path:
      std::cout << "  \\fill[blue] (root.parent anchor) circle[radius=2.5pt];";
      break;
    case cas::NodeType::Value:
      std::cout << "  \\fill[red] (root.parent anchor) circle[radius=2.5pt];";
      break;
    case cas::NodeType::Leaf:
      std::cout << "  \\draw (root.parent anchor) circle[radius=2.5pt];";
      break;
    }
    std::cout << std::endl;
    std::cout << "  \\node at ([yshift=5pt]root.north) {\\ttfamily " << message << "};" << std::endl;
    std::cout << "\\end{forest}" << std::endl;
  }
}

template<>
void cas::Cas<cas::vstring_t>::DumpLatexRoot() {
  root_->DumpLatex(nullptr, 0x00, 1, true);
}
template<>
void cas::Cas<cas::vint32_t>::DumpLatexRoot() {
  root_->DumpLatex(nullptr, 0x00, 1, false);
}
template<>
void cas::Cas<cas::vint64_t>::DumpLatexRoot() {
  root_->DumpLatex(nullptr, 0x00, 1, false);
}


// explicit instantiations to separate header from implementation
template class cas::Cas<cas::vint32_t>;
template class cas::Cas<cas::vint64_t>;
template class cas::Cas<cas::vstring_t>;
