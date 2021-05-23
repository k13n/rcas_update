#include "cas/insertion_helper.hpp"
#include "cas/key_encoding.hpp"
#include <iostream>
#include <cassert>

cas::InsertionHelper::State cas::InsertionHelper::InsertionMatchPathIncremental(
    const std::vector<uint8_t>& path,
    const cas::BinaryQP& qpath,
    size_t len_path,
    State& s){

  const auto& query_path = qpath.bytes_;

  //s.ppos represents how many bytes were matched from the root node up to current node (including in the current node)
  //len_path represents total number of path bytes from the root node to the end of current node that we are visiting
  // s.ppos_ < len_path - check if we have matched everything from the root node till the end of the current node, if we haven't matched everything in the current node, we continue with the check
  while (s.ppos_ < len_path && s.desc_ppos_ < len_path ) {
    if (s.qpos_ < query_path.size() && path[s.ppos_] == query_path[s.qpos_]) {
      // simple pattern symbol matches input symbol
      ++s.ppos_;
      ++s.qpos_;
    }
    // If there is mismatch between path and query_path then we should break and check the cases
    else{
        break;
    }
  }

    return s;
}


cas::InsertionHelper::State cas::InsertionHelper::InsertionMatchPath(
      const std::vector<uint8_t>& path,
      const cas::BinaryQP& query_path){
          cas::InsertionHelper::State s;
          return InsertionMatchPathIncremental(path, query_path, path.size(), s);
      }


void cas::InsertionHelper::State::Dump() {
  std::cout << "ppos_: " << ppos_ << std::endl;
  std::cout << "qpos_: " << qpos_ << std::endl;
  std::cout << "desc_ppos_: " << desc_ppos_ << std::endl;
  std::cout << "desc_qpos_: " << desc_qpos_ << std::endl;
}
