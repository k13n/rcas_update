#include "benchmark/skew_old_experiment.hpp"

#include "cas/cas.hpp"
#include "cas/cas_seq.hpp"
#include "cas/key.hpp"
#include "cas/search_key.hpp"
#include "cas/csv_importer.hpp"

#include <iostream>
#include <chrono>


benchmark::SkewOldExperiment::SkewOldExperiment(
      const std::vector<Dataset> datasets,
      const char dataset_delim,
      const std::string query_path)
  : datasets_(datasets)
  , dataset_delim_(dataset_delim)
  , query_path_(query_path)
{
}


void benchmark::SkewOldExperiment::Run() {
  int nr_repetitions = 100;
  for (const auto& dataset : datasets_) {
    std::cout << "dataset: " << dataset.filename_ << std::endl;
    cas::Cas<cas::vint32_t> index(cas::IndexType::TwoDimensional, {});
    PopulateIndex(index, dataset.filename_);
    RunIndex(index, nr_repetitions);
  }
  PrintOutput();
}


void benchmark::SkewOldExperiment::RunIndex(
    cas::Index<cas::vint32_t>& index, int nr_repetitions) {
  index.Describe();
  std::cout << std::endl;

  std::vector<cas::QueryStats> stats;
  for (int i = 0; i < nr_repetitions; ++i) {
    cas::SearchKey<cas::vint32_t> skey;
    skey.low_  = cas::VINT32_MIN;
    skey.high_ = cas::VINT32_MAX;
    skey.path_ = { query_path_ };
    stats.push_back(index.QueryRuntime(skey));
  }
  results_.push_back(cas::QueryStats::Avg(stats));
}


void benchmark::SkewOldExperiment::PopulateIndex(
    cas::Index<cas::vint32_t>& index, std::string filename) {
  cas::CsvImporter<cas::vint32_t> importer(index, dataset_delim_);
  importer.BulkLoad(filename);
}


void benchmark::SkewOldExperiment::PrintOutput() {
  std::cout
    << "cardinality"
    << ",skew"
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
    double runtime_ms = result.runtime_mus_ / 1000.0;
    std::cout
      << datasets_[i].cardinality_
      << "," << datasets_[i].skew_
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
