#include "benchmark/rcas_query_experiment.hpp"
#include "benchmark/runtime_experiment.hpp"
#include "cas/cas.hpp"
#include "cas/cas_seq.hpp"
#include "cas/key.hpp"
#include "cas/search_key.hpp"
#include "cas/csv_importer.hpp"
#include <iostream>
#include <chrono>


template<class VType>
benchmark::RcasQueryExperiment<VType>::RcasQueryExperiment(
      const std::string dataset_filename,
      const char dataset_delim,
      std::vector<cas::SearchKey<VType>> queries)
  : dataset_filename_(dataset_filename)
  , dataset_delim_(dataset_delim)
  , queries_(queries)
{
}


template<class VType>
void benchmark::RcasQueryExperiment<VType>::Run() {
  std::cout << "Queries:" << std::endl;
  for (size_t i = 0; i < queries_.size(); ++i) {
    std::cout << "q" << i << ": ";
    queries_[i].DumpConcise();
  }
  std::cout << std::endl;

  int nr_repetitions = 100;
  cas::Cas<VType> index(cas::IndexType::TwoDimensional, {});
  PopulateIndex(index);
  RunIndex(index, nr_repetitions);
  PrintOutput();
}


template<class VType>
void benchmark::RcasQueryExperiment<VType>::RunIndex(
    cas::Index<VType>& index, int nr_repetitions) {

  for (auto& skey : queries_) {
    std::vector<cas::QueryStats> stats;
    for (int i = 0; i < nr_repetitions; ++i) {
      stats.push_back(index.QueryRuntime(skey));
    }
    results_.push_back(cas::QueryStats::Avg(stats));
  }
}


template<class VType>
void benchmark::RcasQueryExperiment<VType>::PopulateIndex(
    cas::Index<VType>& index) {
  cas::CsvImporter<VType> importer(index, dataset_delim_);
  uint64_t load_time_mus = importer.BulkLoad(dataset_filename_);
  double load_time_ms = load_time_mus / 1000.0;

  index.Describe();
  std::cout << "Bulk-loading time: " << load_time_ms << " ms" << std::endl;
  std::cout << std::endl;
}


template<class VType>
void benchmark::RcasQueryExperiment<VType>::PrintOutput() {
  std::cout
    << "query"
    << ",runtime_ms"
    << ",runtime_mus"
    << ",nr_matches"
    << ",read_path_nodes_"
    << ",read_value_nodes_"
    << std::endl;

  int i = 0;
  for (const auto& result : results_) {
    double runtime_ms = result.runtime_mus_ / 1000.0;
    std::cout
      << i++
      << "," << runtime_ms
      << "," << result.runtime_mus_
      << "," << result.nr_matches_
      << "," << result.read_path_nodes_
      << "," << result.read_value_nodes_
      << std::endl;
  }
  std::cout << std::endl;
  std::cout << std::endl;
}


// explicit instantiations to separate header from implementation
template class benchmark::RcasQueryExperiment<cas::vint32_t>;
template class benchmark::RcasQueryExperiment<cas::vint64_t>;
template class benchmark::RcasQueryExperiment<cas::vstring_t>;
