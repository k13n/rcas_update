#include "benchmark/runtime_experiment.hpp"

#include "cas/cas.hpp"
#include "cas/cas_seq.hpp"
#include "cas/key.hpp"
#include "cas/search_key.hpp"
#include "cas/csv_importer.hpp"

#include <iostream>
#include <chrono>


template<class VType>
void benchmark::RuntimeExperiment<VType>::RunAllIndexes() {
  int s_max_depth = 21;
  int s_bytes_per_label = 3;
  int nr_repetitions = 5;

  {
    cas::Cas<VType> index(cas::IndexType::TwoDimensional, {});
    RunIndex(index, nr_repetitions);
  } {
    cas::Cas<VType> index(cas::IndexType::ZOrder, {},
        s_max_depth, s_bytes_per_label);
    RunIndex(index, nr_repetitions);
  } {
    cas::Cas<VType> index(cas::IndexType::LevelInterleaving, {});
    RunIndex(index, nr_repetitions);
  } {
    /* cas::Cas<VType> index(cas::IndexType::ByteInterleaving, {}); */
    /* RunIndex(index, nr_repetitions); */
  } {
    cas::Cas<VType> index(cas::IndexType::PathValue, {});
    RunIndex(index, nr_repetitions);
  } {
    cas::Cas<VType> index(cas::IndexType::ValuePath, {});
    RunIndex(index, nr_repetitions);
  } {
    /* cas::Cas<VType> index(cas::IndexType::PathValue, {}, */
    /*     s_max_depth, s_bytes_per_label); */
    /* RunIndex(index, nr_repetitions); */
  } {
    /* cas::Cas<VType> index(cas::IndexType::ValuePath, {}, */
    /*     s_max_depth, s_bytes_per_label); */
    /* RunIndex(index, nr_repetitions); */
  } {
    /* cas::CasSeq<VType> index({}); */
    /* RunIndex(index, 5); */
  }
}


template<class VType>
void benchmark::RuntimeExperiment<VType>::RunIndex(
    cas::Index<VType>& index, int nr_repetitions) {
  PopulateIndex(index);
  results_.clear();
  for (auto& query_path : query_paths_) {
    for (auto& value_range : query_values_) {
      std::vector<cas::QueryStats> stats;

      const auto& t_start = std::chrono::high_resolution_clock::now();
      for (int i = 0; i < nr_repetitions; ++i) {
        cas::SearchKey<VType> skey;
        skey.low_  = std::get<1>(value_range);
        skey.high_ = std::get<2>(value_range);
        skey.path_ = { std::get<1>(query_path) };
        stats.push_back(index.QueryRuntime(skey));
      }
      const auto& t_end = std::chrono::high_resolution_clock::now();
      cas::QueryStats avg =  cas::QueryStats::Avg(stats);
      avg.runtime_mus_ =
        std::chrono::duration_cast<std::chrono::microseconds>(t_end-t_start).count();
      avg.runtime_mus_ /= nr_repetitions;

      results_.push_back(avg);
    }
  }
  std::cout << std::endl;
  index.Describe();
  PrintOutput();
}


template<class VType>
void benchmark::RuntimeExperiment<VType>::PopulateIndex(
    cas::Index<VType>& index) {
  cas::CsvImporter<VType> importer(index, dataset_delim_);
  importer.BulkLoad(dataset_filename_);
}


template<class VType>
void benchmark::RuntimeExperiment<VType>::PrintOutput() {
  std::cout
    << "query_path"
    << ",value_selectivity"
    << ",nr_matches"
    << ",runtime_mus"
    << ",runtime_ms"
    << ",path_matching_mus_"
    << ",value_matching_mus_"
    << ",read_path_nodes_"
    << ",read_value_nodes_"
    << std::endl;

  for (size_t i = 0; i < results_.size(); ++i) {
    const cas::QueryStats& result = results_[i];
    size_t pos_p_predicate = i / query_values_.size();
    size_t pos_v_predicate = i % query_values_.size();

    double runtime_ms = result.runtime_mus_ / 1000.0;

    std::cout
      << std::get<0>(query_paths_[pos_p_predicate])
      << "," << std::get<0>(query_values_[pos_v_predicate])
      << "," << result.nr_matches_
      << "," << result.runtime_mus_
      << "," << runtime_ms
      << "," << result.path_matching_mus_
      << "," << result.value_matching_mus_
      << "," << result.read_path_nodes_
      << "," << result.read_value_nodes_
      << std::endl;
  }
  std::cout << std::endl;
  std::cout << std::endl;
}


// explicit instantiations to separate header from implementation
template class benchmark::RuntimeExperiment<cas::vint32_t>;
template class benchmark::RuntimeExperiment<cas::vint64_t>;
template class benchmark::RuntimeExperiment<cas::vstring_t>;
