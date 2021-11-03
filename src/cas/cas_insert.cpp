#include "cas/cas_insert.hpp"
#include "cas/node0.hpp"
#include <cas/node4.hpp>
#include <cas/node16.hpp>
#include <cas/node48.hpp>
#include <cas/node256.hpp>
#include "cas/utils.hpp"
#include <cassert>
#include <iostream>
#include <functional>
#include <chrono>
#include <cas/bulk_load.hpp>
#include <cas/cas.hpp>


template<class VType>
cas::CasInsert<VType>::CasInsert(cas::Node* root,
        cas::BinarySK& key,
        cas::InsertionHelper& pm,
        cas::did_t did,
        cas::Node* second_index,
        bool isMain,
        cas::MergeMethod merge_method)
  : root_(root)
  , parent_(nullptr)
  , grand_parent_(nullptr)
  , key_(key)
  , pm_(pm)
  , did_ (did)
  , buf_pat_(cas::kMaxPathLength+1, 0x00)
  , buf_val_(cas::kMaxValueLength+1, 0x00)
  , second_index_(second_index)
  , is_main_index_(isMain)
  , merge_method_(merge_method)
{}

template<class VType>
bool cas::CasInsert<VType>::Execute(cas::Node*& root_node, cas::InsertType insertType, cas::Node*& root_node_sec) {
  if (root_node == nullptr) {
    root_node = new Node0();
    cas::Node0 * currNode = static_cast<Node0 *>(root_node);
    currNode->nr_keys_ = 1;
    currNode->separator_pos_ = key_.path_.Size();
    currNode->dids_.reserve(1);
    currNode->dids_.push_back(did_);
    currNode->prefix_= key_.path_.bytes_;
    currNode->prefix_.insert(currNode->prefix_.end(), key_.low_.begin(), key_.low_.end());
    return true;
  }

  const auto& t_start = std::chrono::high_resolution_clock::now();

  State initial_state;
  initial_state.node_ = root_;
  initial_state.parent_type_ = cas::NodeType::Path; // doesn't matter
  initial_state.parent_byte_ = 0x00; // doesn't matter;
  initial_state.len_pat_ = 0;
  initial_state.len_val_ = 0;
  initial_state.vl_pos_ = 0;
  initial_state.vh_pos_ = 0;
  stack_.push_back(initial_state);
  parent_ = nullptr;
  grand_parent_ = nullptr;
  uint8_t parent_disc_byte = 0x00;
  State s;
  std::stack<Node*> traversed_nodes_;

  //we need this to know at what position in the key we start comparison with the next node that we descend to in the tree
  //(we use this values to extract the subvectors of the path and value of key for the Insertion Case 3)
  uint16_t next_node_qpos_ =  0;
  uint16_t next_node_vl_pos_ = 0;

  while (!stack_.empty()) {
    s = stack_.back();
    stack_.pop_back();

    traversed_nodes_.push(s.node_);

    next_node_qpos_ = s.pm_state_.qpos_;
    next_node_vl_pos_ = s.vl_pos_;

    UpdateStats(s);
    // PrepareBuffer takes the current node that we are visiting and takes its path and value bytes and adds them in the buffer. Buffer represents all path and value bytes from the root node to the current node n
    PrepareBuffer(s);

    InsertionMatchPathPrefix(s);
    InsertionMatchValuePrefix(s);
    uint16_t match_val = s.vl_pos_;

    // Here we check: should we Descend or should we go to specific insertion case
    /*line 17*/   if ((s.pm_state_.qpos_ >= key_.path_.bytes_.size()) && (match_val >= key_.low_.size())){
      break;
    }
    /*line 18*/   if ((s.pm_state_.ppos_ < s.len_pat_) || (match_val < s.len_val_)){
      break;
    }

    parent_disc_byte = s.parent_byte_;
    grand_parent_ = parent_;
    parent_ = s.node_;

    /*line 23*/ Descend(s);

    if (s.node_ == nullptr) {
      break;
    }

  }

  //Insert key k

  //Insertion Case 1 - We have complete match, add just reference in the existing node
  /*line 24*/    if ((s.pm_state_.qpos_ >= key_.path_.bytes_.size()) && (s.vl_pos_ >= key_.low_.size())) {
    cas::Node0 * currNode =  static_cast<Node0 *>(s.node_);
    currNode->dids_.push_back(did_);
    currNode->dids_.shrink_to_fit();
    while(!traversed_nodes_.empty()){
      cas::Node *node =  traversed_nodes_.top();
      node->nr_keys_ += 1;
      traversed_nodes_.pop();
    }
    const auto& t_end = std::chrono::high_resolution_clock::now();
    stats_.runtime_mus_ =std::chrono::duration_cast<std::chrono::microseconds>(t_end-t_start).count();

    return true;
  }
  //Insertion Case 2 - Insertion of a new leaf node
  /*line 26*/    else if (s.node_ == nullptr){
    cas::Node0* leaf = new Node0();

    BinaryKey bk;
    bk.path_ = key_.path_.bytes_;
    bk.value_ = key_.low_;
    bk.did_ = did_;

    std::deque<cas::BinaryKey> keys;
    keys.push_front(bk);
    cas::BulkLoad add_Leaf(keys, parent_->type_ == cas::NodeType::Path ? cas::NodeType::Path : cas::NodeType::Value);

    // Make a decision where to increase
    uint16_t path_pos = s.pm_state_.qpos_;
    uint16_t val_pos = s.vl_pos_;
    if(parent_->type_ == cas::NodeType::Path) {
      path_pos++;
    }
    else {
      val_pos++;
    }

    add_Leaf.BuildPrefix(leaf, bk, path_pos, val_pos, bk.Get(cas::NodeType::Path).size(), bk.Get(cas::NodeType::Value).size());
    leaf->dids_.reserve(1);
    leaf->dids_.push_back(did_);
    leaf->nr_keys_ = 1;
    uint8_t key_byte = bk.Get(parent_->type_)[parent_->type_ == cas::NodeType::Path ? s.pm_state_.qpos_ : s.vl_pos_];


    //Does node need to grow check
    //Since the pointer is of the Base class, I don't know which derived class is my current node, Node4 or Node16 ...
    //The easiest way is to just check end values when grow needs to be done
    //*** Possibly will have to change when Delete of keys is added, so the nodes can shrink

    if (parent_->nr_children_ == 4 || parent_->nr_children_ == 16 || parent_->nr_children_ == 48 ){
      Node* extended_parent = parent_->Grow();
      extended_parent->nr_keys_ = parent_->nr_keys_;

      // if the grown node wasn't the root node then we should replace byte pointer in his parent, otherwise root node does not have a parent (i.e. grandparent of the current node) so the replace cannot be done
      // in that case the root node should point to the extended node
      if(grand_parent_ != nullptr){
        grand_parent_->ReplaceBytePointer(parent_disc_byte, extended_parent);
      }else{
        root_node = extended_parent;
      }
      delete parent_;
      parent_ = extended_parent;

    }


    parent_->Put(key_byte, leaf);
    parent_->nr_keys_++;


    // Remove the top node since we may have replaced the last node in the stack since we resized it, in the case we didn't resized it we increased it's nr_keys_ with parent_->nr_keys_++
    traversed_nodes_.pop();
    while(!traversed_nodes_.empty()){
      cas::Node *node =  traversed_nodes_.top();
      node->nr_keys_ += 1;
      traversed_nodes_.pop();
    }


    const auto& t_end = std::chrono::high_resolution_clock::now();
    stats_.runtime_mus_ =
      std::chrono::duration_cast<std::chrono::microseconds>(t_end-t_start).count();

    return true;
  }
  //Insertion Case 3
  else {
    // In this case we don't insert in the main index, we try to insert the key in the auxiliary index. Insertion Case 3 is only performed in the auxiliary index
    if(is_main_index_ == true){

      const auto& t_end = std::chrono::high_resolution_clock::now();
      stats_.runtime_mus_ =
        std::chrono::duration_cast<std::chrono::microseconds>(t_end-t_start).count();

      return false;
    }
    // Case when we are in the auxiliary index. We will first regularly add the key with the Insertion Case 3 in the auxiliary index and then check if we have reached the threshold and if we have then we will just merge main and auxiliary index
    else{

      if(insertType == cas::InsertType::StrictSlow){
        StrictSlowInsertion(s, next_node_qpos_, next_node_vl_pos_);
      }
      else if(insertType == cas::InsertType::LazyFast){
        LazyFastInsertion(s, next_node_qpos_, next_node_vl_pos_);
      }

      //remove node s where the mismatch occurred
      if(!traversed_nodes_.empty()) traversed_nodes_.pop();
      while(!traversed_nodes_.empty()){
        cas::Node *node =  traversed_nodes_.top();
        node->nr_keys_ += 1;
        traversed_nodes_.pop();
      }

      root_node = root_;


      std::stack<Node*> traversed_nodes_sec;

      // TODO 100000000 is threshold, it should be changed to variable and can be any value when the indexes should be merged
      if(root_->nr_keys_ >= 100000000 && is_main_index_ == false){

        mergeIndexes(root_, second_index_, nullptr, nullptr, 0x00, 0x00, cas::NodeType::Path, cas::NodeType::Path, traversed_nodes_sec);

        root_node = nullptr;
        root_node_sec = second_index_;
      }

      const auto& t_end = std::chrono::high_resolution_clock::now();
      stats_.runtime_mus_ =
        std::chrono::duration_cast<std::chrono::microseconds>(t_end-t_start).count();
      return true;
    }

  }
}

template<class VType>
cas::NodeType cas::CasInsert<VType>::DetermineDimension(State &s, uint16_t ip, uint16_t iv) {
  if(ip < s.len_pat_ && iv >= s.len_val_){
    return cas::NodeType::Path;
  }
  else if(ip >= s.len_pat_ && iv < s.len_val_){
    return cas::NodeType::Value;
  }
  else if(s.node_ != root_){
    switch(s.parent_type_){
      case cas::NodeType::Path: return cas::NodeType::Value;
      case cas::NodeType::Value: return cas::NodeType::Path;
      case cas::NodeType::Leaf: return cas::NodeType::Path;

    }
  }
  else{
    switch(s.node_->Type()){
      case cas::NodeType::Path: return cas::NodeType::Value;
      case cas::NodeType::Value: return cas::NodeType::Path;
      case cas::NodeType::Leaf: return cas::NodeType::Path;
    }
  }
}


template<class VType>
void cas::CasInsert<VType>::StrictSlowInsertion(State& s, uint16_t next_node_qpos_, uint16_t next_node_vl_pos_) {

  std::deque<cas::BinaryKey> bkeys_;

  std::vector<uint8_t> path_prefix_;
  std::vector<uint8_t> value_prefix_;
  int leaf_counter = 0;

  size_t node_pat_len = s.node_->separator_pos_;
  size_t path_size = s.node_->prefix_.size();

  if (s.parent_type_ == cas::NodeType::Path){
    // If the node where the mismatch occurred is the root node, the discriminative byte is not in the parent node since the parent does not exist and the discriminative byte is already in the root node
    if(parent_ != nullptr) {
      path_prefix_.push_back(s.parent_byte_);
    }

    for (size_t i=0; i<node_pat_len; i++){
      path_prefix_.push_back(s.node_->prefix_[i]);
    }
    for (size_t i=node_pat_len; i<path_size; i++){
      value_prefix_.push_back(s.node_->prefix_[i]);
    }

  }else if (s.parent_type_ == cas::NodeType::Value){
    for (size_t i=0; i<node_pat_len; i++){
      path_prefix_.push_back(s.node_->prefix_[i]);
    }

    if(parent_ != nullptr) {
      value_prefix_.push_back(s.parent_byte_);
    }

    for (size_t i=node_pat_len; i<path_size; i++){
      value_prefix_.push_back(s.node_->prefix_[i]);
    }

  }

  BinaryKey bk;
  bk.path_.assign(&key_.path_.bytes_[next_node_qpos_],&key_.path_.bytes_[key_.path_.bytes_.size()]);
  bk.value_.assign(&key_.low_[next_node_vl_pos_],&key_.low_[key_.low_.size()]);
  bk.did_ = did_;

  bkeys_.push_back(bk);

  int delete_counter = 0;

  cas::NodeType s_node_type_ = s.node_->type_;
  CollectSubtreeKeys(bkeys_, s.node_, path_prefix_, value_prefix_, leaf_counter);
  cas::Cas<VType>* subtree = new Cas<VType>(cas::IndexType::TwoDimensional, {});

  if(s_node_type_ == cas::NodeType::Leaf){
    // If we want to keep alternation of dimensions we have to do BulkLoad with the alternating NodeType compared to the node parent
    subtree->BulkLoad(bkeys_, s.parent_type_ == cas::NodeType::Path ? cas::NodeType::Value : cas::NodeType::Path);
  }else{
    // We need this check, because we want to keep the alternation of dimensions in the index
    // For example, in the bom.csv index, if we have mismatch in the node 03D3, r/battery$, V and we partition by n.D, that is. V
    // we will lose alternation of the dimensions since the parent of node also has dimension V. The dimension in the node is V since we cannot partition in P but when we add the new node this may change so the
    // partition in the alternating dimension is possible. If the partition is still not possible in the specified dimension, this will be automatically adjusted by the BulkLoad algorithm
    if(s.parent_type_ == cas::NodeType::Path && parent_ != nullptr){
      subtree->BulkLoad(bkeys_, cas::NodeType::Value);
    }else if(s.parent_type_ == cas::NodeType::Value && parent_ != nullptr){
      subtree->BulkLoad(bkeys_, cas::NodeType::Path);
    }else {
      subtree->BulkLoad(bkeys_, cas::NodeType::Value);
    }
  }

  // Now, we have to remove the discriminative byte in the key node we want to insert, since it is now part of the key of its parent node
  // This should be done only if the root_ of the subtree does not become the root_ of the whole tree, that is when the parent_ node is not null
  // If the discriminative byte is the Path byte then we have to shrink the separator position since we remove the first element of the prefix as a discriminative byte
  if (s.parent_type_ == cas::NodeType::Path && parent_ != nullptr) {
    subtree->root_->prefix_.erase(subtree->root_->prefix_.begin());
    subtree->root_->separator_pos_--;
  }
  else if (s.parent_type_ == cas::NodeType::Value && parent_ != nullptr){
    subtree->root_->prefix_.erase(subtree->root_->prefix_.begin()+subtree->root_->separator_pos_);
  }
  subtree->root_->prefix_.shrink_to_fit();

  // Parent can only be null if the mismatch occurred in the root node, so the root node is replaced with the new subtree
  if(parent_ == nullptr){
    root_ = subtree->root_;
  }else{
    parent_->ReplaceBytePointer(s.parent_byte_, subtree->root_);
  }
  subtree->root_ = nullptr;
  delete subtree;
}

template<class VType>
void cas::CasInsert<VType>::LazyFastInsertion(State &s, uint16_t next_node_qpos_, uint16_t next_node_vl_pos_) {

  //Determine dimension
  size_t node_pat_len = s.node_->separator_pos_;
  size_t node_val_len = s.node_->prefix_.size() - s.node_->separator_pos_;
  std::vector<uint8_t> node_path_;
  std::vector<uint8_t> node_val_;

  //We have to increase node_pat_len/node_val_len because the size of path/value that can be found in the prefix is not correct since in comparison of nodes the discriminative byte is calculated as being present in the prefix, but it is not
  //Now we have real path and value prefix of the current node
  //This should be done only if the mismatch did not happen in the root node, since the root node does not have prefix in the parent node
  if(s.parent_type_ == NodeType::Path){
    if(parent_ != nullptr) {
      node_pat_len += 1;
    }
    node_path_.reserve(node_pat_len);

    if(parent_ != nullptr) {
      node_path_.push_back(s.parent_byte_);
    }
    node_val_.reserve(node_val_len);

  }else{
    node_path_.reserve(node_pat_len);

    if(parent_ != nullptr) {
      node_val_len += 1;
    }
    node_val_.reserve(node_val_len);
    if(parent_ != nullptr) {
      node_val_.push_back(s.parent_byte_);
    }
  }

  node_path_.insert(node_path_.end(), &s.node_->prefix_[0], &s.node_->prefix_[s.node_->separator_pos_]);
  node_val_.insert(node_val_.end(), &s.node_->prefix_[s.node_->separator_pos_], &s.node_->prefix_[s.node_->prefix_.size()]);

  uint16_t ip = s.pm_state_.ppos_ - next_node_qpos_;
  uint16_t iv = s.vl_pos_ - next_node_vl_pos_;

  // New parent node of n
  Node4* np_prim = new Node4(DetermineDimension(s, s.pm_state_.ppos_, s.vl_pos_));
  uint8_t np_prim_disc_byte;

  std::vector<uint8_t> np_prim_path_;
  std::vector<uint8_t> np_prim_val_;
  std::vector<uint8_t> new_curr_node_path_;
  std::vector<uint8_t> new_curr_node_val_;

  //If we matched something in the path i.e. if curr_node and node_to_be_inserted have some common path
  if(ip > 0){
    np_prim_path_.reserve(ip);

    np_prim_path_.insert(np_prim_path_.end(), &node_path_[0], &node_path_[ip]);
  }

  //If there is some part of the path that is not matched, it becomes new path of the current node n
  if((node_path_.size() - ip) > 0) {
    new_curr_node_path_.reserve(node_path_.size() - ip);
    new_curr_node_path_.insert(new_curr_node_path_.end(), &node_path_[ip],
        &node_path_[node_path_.size()]);
  }

  //If we matched something in the value i.e. if curr_node and node_to_be_inserted have some common value
  if(iv > 0){
    np_prim_val_.reserve(iv);
    np_prim_val_.insert(np_prim_val_.end(), &node_val_[0], &node_val_[iv]);
  }

  //If there is some part of the value that is not matched, it becomes new value of the current node n
  if((node_val_.size() - iv) > 0) {
    new_curr_node_val_.reserve(node_val_.size() - iv);
    new_curr_node_val_.insert(new_curr_node_val_.end(), &node_val_[iv],
        &node_val_[node_val_.size()]);
  }

  // We subtract -1 because either np_prim_path_ or np_prim_val_ contains also discriminative byte that is actually kept in the parent node
  // We shorten the vector which has discriminative byte and that node would not become the next root
  if(s.parent_type_ == NodeType::Path && parent_ != nullptr) {
    np_prim_disc_byte = np_prim_path_[0];
    //Then we remove the first element from the path
    np_prim_path_.erase(np_prim_path_.begin());
  }else if(s.parent_type_ == cas::NodeType::Value && parent_ != nullptr){
    np_prim_disc_byte = np_prim_val_[0];
    //Then we remove the first element from the value
    np_prim_val_.erase(np_prim_val_.begin());
  }
  np_prim->separator_pos_ = np_prim_path_.size();
  np_prim->prefix_.insert(np_prim->prefix_.end(), np_prim_path_.begin(), np_prim_path_.end());
  np_prim->prefix_.insert(np_prim->prefix_.end(), np_prim_val_.begin(), np_prim_val_.end());

  //Update the substrings in node n
  std::vector<uint8_t> new_curr_node_prefix_;
  uint8_t new_curr_node_disc_byte;

  //  We shorten the vector which has discriminative byte
  if(np_prim->Type() == NodeType::Path) {
    new_curr_node_disc_byte = new_curr_node_path_[0];
    //Then we remove the first element from the path
    new_curr_node_path_.erase(new_curr_node_path_.begin());
  }else{
    new_curr_node_disc_byte = new_curr_node_val_[0];
    //Then we remove the first element from the value
    new_curr_node_val_.erase(new_curr_node_val_.begin());
  }

  new_curr_node_prefix_.reserve(new_curr_node_path_.size() + new_curr_node_val_.size());
  new_curr_node_prefix_.insert(new_curr_node_prefix_.end(), new_curr_node_path_.begin(), new_curr_node_path_.end());
  new_curr_node_prefix_.insert(new_curr_node_prefix_.end(), new_curr_node_val_.begin(), new_curr_node_val_.end());
  s.node_->separator_pos_ = new_curr_node_path_.size();
  s.node_->prefix_ = new_curr_node_prefix_;

  //New sibling node of n
  Node0* node_sibling = new Node0();
  uint8_t node_sibling_disc_byte;
  node_sibling->nr_keys_ = 1;

  std::vector<uint8_t> sibling_node_path_;
  std::vector<uint8_t> sibling_node_val_;

  if((key_.path_.Size() - s.pm_state_.qpos_) > 0) {
    sibling_node_path_.reserve(key_.path_.Size() - s.pm_state_.qpos_);
    sibling_node_path_.insert(sibling_node_path_.end(), &key_.path_.bytes_[s.pm_state_.qpos_],
        &key_.path_.bytes_[key_.path_.Size()]);
  }

  if((key_.low_.size() - s.vl_pos_) > 0) {
    sibling_node_val_.reserve(key_.low_.size() - s.vl_pos_);
    sibling_node_val_.insert(sibling_node_val_.end(), &key_.low_[s.vl_pos_],
        &key_.low_[key_.low_.size()]);
  }
  //  We shorten the vector which has discriminative byte
  if(np_prim->Type() == NodeType::Path) {
    node_sibling_disc_byte = sibling_node_path_[0];
    //Then we remove the first element from the path
    sibling_node_path_.erase(sibling_node_path_.begin());
  }else{
    node_sibling_disc_byte = sibling_node_val_[0];
    //Then we remove the first element from the value
    sibling_node_val_.erase(sibling_node_val_.begin());
  }

  node_sibling->separator_pos_ = sibling_node_path_.size();
  node_sibling->prefix_.reserve(sibling_node_path_.size() + sibling_node_val_.size());
  node_sibling->prefix_.insert(node_sibling->prefix_.end(), sibling_node_path_.begin(), sibling_node_path_.end());
  node_sibling->prefix_.insert(node_sibling->prefix_.end(), sibling_node_val_.begin(), sibling_node_val_.end());
  node_sibling->dids_.push_back(did_);

  //Install new nodes
  np_prim->Put(node_sibling_disc_byte, node_sibling);
  np_prim->Put(new_curr_node_disc_byte, s.node_);

  np_prim->nr_keys_ = node_sibling->nr_keys_ + s.node_->nr_keys_;

  if(parent_ == nullptr){
    root_ = np_prim;
  }else{
    parent_->ReplaceBytePointer(np_prim_disc_byte, np_prim);
  }
}

template<class VType>
void cas::CasInsert<VType>::CollectSubtreeKeys(std::deque<cas::BinaryKey>& bkeys_,
    cas::Node* node,
    std::vector<uint8_t> path_prefix_,
    std::vector<uint8_t> value_prefix_,
    int& leaf_counter){

  // Current node is a Leaf node
  if(node->nr_children_ == 0){
    cas::Node0* leaf_Node = static_cast<cas::Node0 *> (node);

    for (size_t n = 0; n < leaf_Node->dids_.size(); n++) {
      BinaryKey bk;
      bk.path_ = path_prefix_;
      bk.value_ = value_prefix_;
      bk.did_ = leaf_Node->dids_[n];
      bkeys_.push_back(bk);
      leaf_counter++;
    }
    delete node;
    node = nullptr;
  }
  //Current node has children
  else{
    node->ForEachChild([&](uint8_t byte, cas::Node& child) -> bool {
        std::vector<uint8_t> curr_path_prefix_(path_prefix_);
        std::vector<uint8_t> curr_value_prefix_(value_prefix_);

        size_t node_pat_len = child.separator_pos_;
        size_t path_size = child.prefix_.size();

        if (node->type_ == cas::NodeType::Path){
        curr_path_prefix_.push_back(byte);
        for (size_t i=0; i<node_pat_len; i++){
        curr_path_prefix_.push_back(child.prefix_[i]);
        }

        for (size_t i=node_pat_len; i<path_size; i++){
        curr_value_prefix_.push_back(child.prefix_[i]);
        }

        }else if (node->type_ == cas::NodeType::Value){
        for (size_t i=0; i<node_pat_len; i++){
        curr_path_prefix_.push_back(child.prefix_[i]);
        }

        curr_value_prefix_.push_back(byte);
        for (size_t i=node_pat_len; i<path_size; i++){
          curr_value_prefix_.push_back(child.prefix_[i]);
        }
        }

        CollectSubtreeKeys(bkeys_, &child, curr_path_prefix_, curr_value_prefix_, leaf_counter);
        return true;
    });
    delete node;
    node = nullptr;
  }
  return;
}


  template<class VType>                   //node_prim auxiliary_index, node_sec main_index, parent_node_prim = nullptr and parent_node_sec = nullptr at the beginning
void cas::CasInsert<VType>::mergeIndexes(
    cas::Node* node_prim,
    cas::Node* node_sec,
    cas::Node* parent_node_prim,
    cas::Node* parent_node_sec,
    uint8_t parent_byte_prim,
    uint8_t parent_byte_sec,
    NodeType parent_type_prim,
    NodeType parent_type_sec,
    std::stack<cas::Node*> traversed_nodes_sec)
{

  traversed_nodes_sec.push(node_sec);

  std::vector<uint8_t> path_prim_;
  std::vector<uint8_t> val_prim_;
  std::vector<uint8_t> path_sec_;
  std::vector<uint8_t> val_sec_;

  path_prim_.reserve(node_prim->separator_pos_);
  path_prim_.insert(path_prim_.end(), node_prim->prefix_.begin(), node_prim->prefix_.begin() + node_prim->separator_pos_);
  val_prim_.reserve(node_prim->prefix_.size() - node_prim->separator_pos_);
  val_prim_.insert(val_prim_.end(), node_prim->prefix_.begin() + node_prim->separator_pos_, node_prim->prefix_.end());

  path_sec_.reserve(node_sec->separator_pos_);
  path_sec_.insert(path_sec_.end(), node_sec->prefix_.begin(), node_sec->prefix_.begin() + node_sec->separator_pos_);
  val_sec_.reserve(node_sec->prefix_.size() - node_sec->separator_pos_);
  val_sec_.insert(val_sec_.end(), node_sec->prefix_.begin() + node_sec->separator_pos_, node_sec->prefix_.end());

  uint16_t gP = 0;
  uint16_t gV = 0;
  uint16_t iP = 0;
  uint16_t iV = 0;

  //check the path match
  while (gP < path_prim_.size() && iP < path_sec_.size() && path_prim_[gP] == path_sec_[iP]) {
    // simple pattern symbol matches input symbol
    ++gP;
    ++iP;
  }

  //check the value match
  while (gV < val_prim_.size() && iV < val_sec_.size() && val_prim_[gV] == val_sec_[iV]) {
    // simple pattern symbol matches input symbol
    ++gV;
    ++iV;
  }

  // add some check to switch between merging the indexes by complete collection of two indexes and by traversing the indexes and merging only subtrees where they mismatch
  if (merge_method_ == cas::MergeMethod::Slow){
    collectMainAndAuxIndexAndRebuildSubtree(node_prim, node_sec, parent_node_prim, parent_node_sec, parent_byte_prim, parent_byte_sec, parent_type_prim, parent_type_sec, traversed_nodes_sec);
    std::cout << "Collect and rebuild from roots" << std::endl;
    std::cout << "Entered if(true)" << std::endl;
  }else{
    //Check the cases
    //Case 3, we have mismatch of the nodes
    //Subtree of both nodes need to be collected and do BulkLoad on all collected keys
    if(gP < path_prim_.size() || iP < path_sec_.size() || gV < val_prim_.size() || iV < val_sec_.size()){


      collectMainAndAuxIndexAndRebuildSubtree(node_prim, node_sec, parent_node_prim, parent_node_sec, parent_byte_prim, parent_byte_sec, parent_type_prim, parent_type_sec, traversed_nodes_sec);
    }

    //Case 1, we have match of the nodes
    //Check whether we can Descend from the auxiliary node to the node in the main index
    else if(gP >= path_prim_.size() && iP >= path_sec_.size() && gV >= val_prim_.size() && iV >= val_sec_.size()){
      //current parent nodes
      cas::Node* np_prim = node_prim;
      cas::Node* np_sec = node_sec;

      // Want to find a child node from auxiliary index with the same discriminative byte in the main index
      node_prim->ForEachChild([&](uint8_t byte, cas::Node& child) -> bool {
          cas::Node* child_sec = node_sec->LocateChild(byte);
          if (child_sec != nullptr) {
          mergeIndexes(&child, child_sec, node_prim, node_sec, byte, byte, np_prim->Type(), np_sec->Type(), traversed_nodes_sec);
          }
          // Case 2, For the child subtree in the auxiliary index we cannot Descend in the main index, so we just add the child auxiliary subtree as a child subtree in the main index
          //If we do an insert by first looking at whether we can insert a key into the Main index, we will never enter in Auxiliary index here, because the key as a new Leaf node could already have been inserted into the Main index
          else{
          // node_sec in the main index should be expanded
          if (node_sec->nr_children_ == 4 || node_sec->nr_children_ == 16 || node_sec->nr_children_ == 48){
          Node* extended_parent = node_sec->Grow();
          extended_parent->nr_keys_ = node_sec->nr_keys_;

          // if the grown node wasn't the root node then we should replace byte pointer in his parent, otherwise root node does not have a parent (i.e. grandparent of the current node) so the replace cannot be done
          // in that case the root node should point to the extended node
          if(parent_node_sec != nullptr){
          parent_node_sec->ReplaceBytePointer(parent_byte_sec, extended_parent);
          }else{
          second_index_ = extended_parent;
          }
          delete node_sec;
          node_sec = extended_parent;
          }

          node_sec->Put(byte, &child);
          node_sec->nr_keys_ += child.nr_keys_;
          // auxiliary index must not point anymore on the subtree that was moved to the main index
          // here we need to delete pointer in the parent of node_prim since the node_prim subtree was completely moved to the main index and not deleted
          // node_prim->nr_keys_ -= child.nr_keys_;

          node_prim->DeleteNode(byte);

          //Update nr_keys in the subtree
          //remove node_sec where the mismatch occurred
          if(!traversed_nodes_sec.empty()) traversed_nodes_sec.pop();
          while(!traversed_nodes_sec.empty()){
            cas::Node *node =  traversed_nodes_sec.top();
            node->nr_keys_  = node->nr_keys_ + child.nr_keys_;
            traversed_nodes_sec.pop();
          }
          }
          return true;
      });
      delete node_prim;
      node_prim = nullptr;
    }
  }
}


template<class VType>
void cas::CasInsert<VType>::collectMainAndAuxIndexAndRebuildSubtree(cas::Node *node_prim, cas::Node *node_sec,
    cas::Node *parent_node_prim, cas::Node *parent_node_sec,
    uint8_t parent_byte_prim, uint8_t parent_byte_sec,
    cas::NodeType parent_type_prim,
    cas::NodeType parent_type_sec,
    std::stack<cas::Node *> traversed_nodes_sec) {

  //Collection of subtree in auxiliary index
  std::deque<cas::BinaryKey> bkeys_;
  int leaf_counter_prim = 0;

  std::vector<uint8_t> path_prefix_prim;
  std::vector<uint8_t> value_prefix_prim;

  size_t node_pat_len = node_prim->separator_pos_;
  size_t prefix_size = node_prim->prefix_.size();

  if (parent_type_prim == cas::NodeType::Path){
    // If the node where the mismatch occurred is the root node, the discriminative byte is not in the parent node
    // since the parent does not exist and the discriminative byte is already in the root node
    if(parent_node_prim != nullptr) {
      path_prefix_prim.push_back(parent_byte_prim);
    }

    for (size_t i=0; i<node_pat_len; i++){
      path_prefix_prim.push_back(node_prim->prefix_[i]);
    }
    for (size_t i=node_pat_len; i<prefix_size; i++){
      value_prefix_prim.push_back(node_prim->prefix_[i]);
    }

  }else if (parent_type_prim == cas::NodeType::Value){
    for (size_t i=0; i<node_pat_len; i++){
      path_prefix_prim.push_back(node_prim->prefix_[i]);
    }

    if(parent_node_prim != nullptr) {
      value_prefix_prim.push_back(parent_byte_prim);
    }

    for (size_t i=node_pat_len; i<prefix_size; i++){
      value_prefix_prim.push_back(node_prim->prefix_[i]);
    }

  }

  CollectSubtreeKeys(bkeys_, node_prim, path_prefix_prim, value_prefix_prim, leaf_counter_prim);

  //Collection of subtree in main index
  int leaf_counter_sec = 0;

  std::vector<uint8_t> path_prefix_sec;
  std::vector<uint8_t> value_prefix_sec;

  node_pat_len = node_sec->separator_pos_;
  prefix_size = node_sec->prefix_.size();

  if (parent_type_sec == cas::NodeType::Path){
    // If the node where the mismatch occurred is the root node, the discriminative byte is not in the parent node
    // since the parent does not exist and the discriminative byte is already in the root node
    if(parent_node_sec != nullptr) {
      path_prefix_sec.push_back(parent_byte_sec);
    }

    for (size_t i=0; i<node_pat_len; i++){
      path_prefix_sec.push_back(node_sec->prefix_[i]);
    }
    for (size_t i=node_pat_len; i<prefix_size; i++){
      value_prefix_sec.push_back(node_sec->prefix_[i]);
    }

  }else if (parent_type_sec == cas::NodeType::Value){
    for (size_t i=0; i<node_pat_len; i++){
      path_prefix_sec.push_back(node_sec->prefix_[i]);
    }

    if(parent_node_sec != nullptr) {
      value_prefix_sec.push_back(parent_byte_sec);
    }

    for (size_t i=node_pat_len; i<prefix_size; i++){
      value_prefix_sec.push_back(node_sec->prefix_[i]);
    }

  }

  cas::NodeType node_type_sec = node_sec->Type();
  size_t node_sec_nr_keys = node_sec->nr_keys_;
  CollectSubtreeKeys(bkeys_, node_sec, path_prefix_sec, value_prefix_sec, leaf_counter_sec);


  cas::Cas<VType>* subtree = new Cas<VType>(cas::IndexType::TwoDimensional, {});

  if(node_type_sec == cas::NodeType::Leaf){
    // If we want to keep alternation of dimensions we have to do BulkLoad with the alternating NodeType compared to the node parent
    subtree->BulkLoad(bkeys_, parent_type_sec == cas::NodeType::Path ? cas::NodeType::Value : cas::NodeType::Path);
  }else{
    // We need this check, because we want to keep the alternation of dimensions in the index
    // For example, in the bom.csv index, if we have mismatch in the node 03D3, r/battery$, V and we partition by n.D, that is. V
    // we will lose alternation of the dimensions since the parent of node also has dimension V. The dimension in the node is V since we cannot partition in P but when we add the new node this may change so the
    // partition in the alternating dimension is possible. If the partition is still not possible in the specified dimension, this will be automatically adjusted by the BulkLoad algorithm
    if(parent_type_sec == cas::NodeType::Path /*&& s.node_->type_ == cas::NodeType::Path*/ && parent_node_sec != nullptr){
      subtree->BulkLoad(bkeys_, cas::NodeType::Value);
    }else if(parent_type_sec == cas::NodeType::Value /*&& s.node_->type_ == cas::NodeType::Value*/ && parent_node_sec != nullptr){
      subtree->BulkLoad(bkeys_, cas::NodeType::Path);
    }else {
      subtree->BulkLoad(bkeys_, cas::NodeType::Value);
    }
  }

  // Now, we have to remove the discriminative byte in the key node we want to insert, since it is now part of the key of its parent node
  // This should be done only if the root_ of the subtree does not become the root_ of the whole tree, that is when the parent_ node is not null
  // If the discriminative byte is the Path byte then we have to shrink the separator position since we remove the first element of the prefix as a discriminative byte
  if (parent_type_sec == cas::NodeType::Path && parent_node_sec != nullptr) {
    subtree->root_->prefix_.erase(subtree->root_->prefix_.begin());
    subtree->root_->separator_pos_--;
  }
  else if (parent_type_sec == cas::NodeType::Value && parent_node_sec != nullptr){
    subtree->root_->prefix_.erase(subtree->root_->prefix_.begin()+subtree->root_->separator_pos_);
  }
  subtree->root_->prefix_.shrink_to_fit();

  // Parent can only be null if the mismatch occurred in the root node, so the root node is replaced with the new subtree
  if(parent_node_sec == nullptr){
    second_index_ = subtree->root_;
  }else{
    parent_node_sec->ReplaceBytePointer(parent_byte_sec, subtree->root_);

    //Update nr_keys in the subtree
    //remove node_sec where the mismatch occured
    if(!traversed_nodes_sec.empty()) { traversed_nodes_sec.pop(); }
    //
    size_t nr_keys_difference_sec = subtree->root_->nr_keys_ - node_sec_nr_keys;
    //
    while(!traversed_nodes_sec.empty()){
      cas::Node *node =  traversed_nodes_sec.top();
      node->nr_keys_  = node->nr_keys_ + (subtree->root_->nr_keys_ - node_sec_nr_keys);
      traversed_nodes_sec.pop();
    }
  }

  // If we collected all nodes from the auxiliary index, the root of the auxiliary index has to be set to nullptr
  // otherwise we do not have to do anything because the auxiliary index nodes are deleted as they are collected
  if(parent_node_prim == nullptr) {
    root_ = nullptr;
  }

  subtree->root_ = nullptr;
  delete subtree;
}


template<class VType>
void cas::CasInsert<VType>::DeleteTreeNodesRecursively(cas::Node *node, int& delete_counter) {
  if (node == nullptr) {
    return;
  }
  node->ForEachChild([&](uint8_t, cas::Node& child) -> bool {
      DeleteTreeNodesRecursively(&child, delete_counter);
      return true;
      });
  delete node;
  node = nullptr;
  delete_counter++;
}



template<class VType>
void cas::CasInsert<VType>::UpdateStats(State& s) {
  switch (s.node_->type_) {
    case cas::NodeType::Path:
      ++stats_.read_path_nodes_;
      break;
    case cas::NodeType::Value:
      ++stats_.read_value_nodes_;
      break;
    case cas::NodeType::Leaf:
      break;
  }
}


template<class VType>
void cas::CasInsert<VType>::PrepareBuffer(State& s) {
  if (s.node_ != root_) {
    switch (s.parent_type_) {
      case cas::NodeType::Path:
        buf_pat_[s.len_pat_] = s.parent_byte_;
        ++s.len_pat_;
        break;
      case cas::NodeType::Value:
        buf_val_[s.len_val_] = s.parent_byte_;
        ++s.len_val_;
        break;
      case cas::NodeType::Leaf:
        assert(false);
        break;
    }
  }
  size_t node_pat_len = s.node_->separator_pos_;
  size_t node_val_len = s.node_->prefix_.size() - s.node_->separator_pos_;
  std::memcpy(&buf_pat_[s.len_pat_], &s.node_->prefix_[0],
      node_pat_len);
  std::memcpy(&buf_val_[s.len_val_], &s.node_->prefix_[s.node_->separator_pos_],
      node_val_len);

  s.len_pat_ += node_pat_len;
  s.len_val_ += node_val_len;
}

template<class VType>
cas::InsertionHelper::State
cas::CasInsert<VType>::InsertionMatchPathPrefix(State& s) {
  return pm_.InsertionMatchPathIncremental(buf_pat_, key_.path_,
      s.len_pat_, s.pm_state_);
}

template<class VType>
void cas::CasInsert<VType>::InsertionMatchValuePrefix(State& s){

  // match as much as possible of key_.low_
  while (s.vl_pos_ < key_.low_.size() &&
      s.vl_pos_ < s.len_val_ &&
      buf_val_[s.vl_pos_] == key_.low_[s.vl_pos_]) {
    ++s.vl_pos_;
    ++s.vh_pos_;
  }

}


template<class VType>
void cas::CasInsert<VType>::Descend(State& s) {
  switch (s.node_->type_) {
    case cas::NodeType::Path:
      DescendPathNode(s);
      break;
    case cas::NodeType::Value:
      DescendValueNode(s);
      break;
    case cas::NodeType::Leaf:
      //    assert(false);
      s.node_ = nullptr;
      break;
  }
}


template<class VType>
void cas::CasInsert<VType>::DescendPathNode(State& s) {

  // we are looking for exactly one child
  uint8_t byte = key_.path_.bytes_[s.pm_state_.qpos_];

  cas::Node* child = s.node_->LocateChild(byte);
  if (child != nullptr) {

    stack_.push_back({
        .node_        = child,
        .parent_type_ = s.node_->type_,
        .parent_byte_ = byte,
        .len_pat_     = s.len_pat_,
        .len_val_     = s.len_val_,
        .pm_state_    = s.pm_state_,
        .vl_pos_      = s.vl_pos_,
        .vh_pos_      = s.vh_pos_,
        });
  }else{
    s.node_ = child;
  }
}


template<class VType>
void cas::CasInsert<VType>::DescendValueNode(State& s) {
  uint8_t low  = (s.vl_pos_ == s.len_val_) ? key_.low_[s.vl_pos_]  : 0x00;
  uint8_t high = (s.vh_pos_ == s.len_val_) ? key_.high_[s.vh_pos_] : 0xFF;

  s.node_->ForEachChild(low, high, [&](uint8_t byte, cas::Node& child) -> bool {
      stack_.push_back({
          .node_        = &child,
          .parent_type_ = s.node_->type_,
          .parent_byte_ = byte,
          .len_pat_     = s.len_pat_,
          .len_val_     = s.len_val_,
          .pm_state_    = s.pm_state_,
          .vl_pos_      = s.vl_pos_,
          .vh_pos_      = s.vh_pos_,
          });
      return true;
      });

  if(stack_.empty()){
    s.node_ = nullptr;
  }
}

template<>
bool cas::CasInsert<cas::vint32_t>::IsCompleteValue(State& s) {
  return s.len_val_ == sizeof(cas::vint32_t);
}
template<>
bool cas::CasInsert<cas::vint64_t>::IsCompleteValue(State& s) {
  return s.len_val_ == sizeof(cas::vint64_t);
}
template<>
bool cas::CasInsert<cas::vstring_t>::IsCompleteValue(State& s) {
  if (s.len_val_ <= 1) {
    // the null byte alone is no complete value
    return false;
  }
  return buf_val_[s.len_val_ - 1] == '\0';
}


template<class VType>
void cas::CasInsert<VType>::State::Dump() {
  std::cout << "node: " << node_ << std::endl;
  switch (parent_type_) {
    case cas::NodeType::Path:
      std::cout << "parent_type_: Path" << std::endl;
      break;
    case cas::NodeType::Value:
      std::cout << "parent_type_: Value" << std::endl;
      break;
    case cas::NodeType::Leaf:
      assert(false);
      break;
  }
  printf("parent_byte_: 0x%02X\n", (unsigned char) parent_byte_);
  std::cout << "len_val_: " << len_val_ << std::endl;
  std::cout << "len_pat_: " << len_pat_ << std::endl;
  pm_state_.Dump();
  std::cout << "vl_pos_: " << vl_pos_ << std::endl;
  std::cout << "vh_pos_: " << vh_pos_ << std::endl;
}

template<class VType>
void cas::CasInsert<VType>::DumpState(State& s) {
  std::cout << "buf_pat_: ";
  cas::Utils::DumpChars(buf_pat_, s.len_pat_);
  std::cout << std::endl;
  std::cout << "buf_pat_: ";
  cas::Utils::DumpHexValues(buf_pat_, s.len_pat_);
  std::cout << std::endl;
  std::cout << "buf_val_: ";
  cas::Utils::DumpHexValues(buf_val_, s.len_val_);
  std::cout << std::endl;
  std::cout << "Sate:" << std::endl;
  s.Dump();
  std::cout << std::endl;
}

template<class VType>
cas::Node *cas::CasInsert<VType>::getSecondIndex() const {
  return second_index_;
}

// explicit instantiations to separate header from implementation
template class cas::CasInsert<cas::vint32_t>;
template class cas::CasInsert<cas::vint64_t>;
template class cas::CasInsert<cas::vstring_t>;
