#include "cas/query_stats.hpp"
#include <iostream>


void cas::QueryStats::Dump() const {
  std::cout << "QueryStats" << std::endl;
  std::cout << "Matches: " << nr_matches_ << std::endl;
  std::cout << "Read Path Nodes: " << read_path_nodes_ << std::endl;
  std::cout << "Read Value Nodes: " << read_value_nodes_ << std::endl;
  std::cout << "Runtime (mus): " << runtime_mus_ << std::endl;
  std::cout << "Runtime in Main index (mus): " << runtime_main_mus_ << std::endl;
  std::cout << "Runtime in Auxiliary index (mus): " << runtime_aux_mus_ << std::endl;
  std::cout << "Value Matching Runtime (mus): " << value_matching_mus_ << std::endl;
  std::cout << "Path Matching Runtime (mus):  " << path_matching_mus_ << std::endl;
  std::cout << "DID Matching Runtime (mus):   " << did_matching_mus_ << std::endl;
  std::cout << std::endl;
}


cas::QueryStats cas::QueryStats::Avg(const std::vector<cas::QueryStats>& stats) {
  cas::QueryStats result;
  int32_t nr_insert_time_main = 1;
  int32_t nr_insert_time_aux = 1;
  result.nr_matches_ = 0;
  result.read_path_nodes_ = 0;
  result.read_value_nodes_ = 0;
  result.runtime_mus_ = 0;
  result.runtime_main_mus_ = 0;
  result.runtime_aux_mus_ = 0;
  result.value_matching_mus_ = 0;
  result.path_matching_mus_ = 0;
  result.did_matching_mus_ = 0;
  if (!stats.empty()) {
    for (const auto& stat : stats) {
      result.nr_matches_ += stat.nr_matches_;
      result.read_path_nodes_ += stat.read_path_nodes_;
      result.read_value_nodes_ += stat.read_value_nodes_;
      result.runtime_mus_ += stat.runtime_mus_;

      if(stat.runtime_main_mus_ > 0){
        nr_insert_time_main++;
        result.runtime_main_mus_ += stat.runtime_main_mus_;
      }
      if(stat.runtime_aux_mus_ > 0) {
        nr_insert_time_aux++;
        result.runtime_aux_mus_ += stat.runtime_aux_mus_;
      }
      result.value_matching_mus_ += stat.value_matching_mus_;
      result.path_matching_mus_ += stat.path_matching_mus_;
      result.did_matching_mus_ += stat.did_matching_mus_;
    }
    result.nr_matches_  = static_cast<int32_t>(result.nr_matches_  / stats.size());
    result.read_path_nodes_ = static_cast<int32_t>(result.read_path_nodes_  / stats.size());
    result.read_value_nodes_ = static_cast<int32_t>(result.read_value_nodes_  / stats.size());
    result.runtime_mus_ = static_cast<int64_t>(result.runtime_mus_ / stats.size());

    if(nr_insert_time_main > 1) { nr_insert_time_main--; }
    result.runtime_main_mus_ = static_cast<int64_t>(result.runtime_main_mus_ / nr_insert_time_main);

    if(nr_insert_time_aux > 1) { nr_insert_time_aux--; }
    result.runtime_aux_mus_ = static_cast<int64_t>(result.runtime_aux_mus_ / nr_insert_time_aux);

    result.value_matching_mus_ = static_cast<int64_t>(result.value_matching_mus_ / stats.size());
    result.path_matching_mus_ = static_cast<int64_t>(result.path_matching_mus_ / stats.size());
    result.did_matching_mus_ = static_cast<int64_t>(result.did_matching_mus_ / stats.size());
  }
  return result;
}
