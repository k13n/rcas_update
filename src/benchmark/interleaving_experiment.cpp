#include "benchmark/interleaving_experiment.hpp"
#include "cas/cas.hpp"
#include "cas/csv_importer.hpp"
#include "cas/interleaving_score.hpp"
#include <iostream>
#include <chrono>


void benchmark::InterleavingExperiment::Run() {
  for (const auto& dataset : datasets_) {
    std::vector<double> results;
    for (const auto& approach : approaches_) {
      results.push_back(RunIndex(dataset, approach));
    }
    results_.push_back(results);
    std::cout << "finished dataset: "
      << dataset.filename_
      << " / " << dataset.data_type_
      << std::endl;
  }
  PrintOutput();
}


double benchmark::InterleavingExperiment::RunIndex(
    const Dataset& dataset, const Approach& approach) {
  if (dataset.data_type_ == "vint32_t") {
    if (approach.use_surrogate_) {
      cas::Cas<cas::vint32_t> index(approach.type_, {},
          approach.max_depth_, approach.bytes_per_label_);
      return ComputeScore(dataset, index);
    } else {
      cas::Cas<cas::vint32_t> index(approach.type_, {});
      return ComputeScore(dataset, index);
    }
  } else if (dataset.data_type_ == "vint64_t") {
    if (approach.use_surrogate_) {
      cas::Cas<cas::vint64_t> index(approach.type_, {},
          approach.max_depth_, approach.bytes_per_label_);
      return ComputeScore(dataset, index);
    } else {
      cas::Cas<cas::vint64_t> index(approach.type_, {});
      return ComputeScore(dataset, index);
    }
  } else if (dataset.data_type_ == "vstring_t") {
    if (approach.use_surrogate_) {
      cas::Cas<cas::vstring_t> index(approach.type_, {},
          approach.max_depth_, approach.bytes_per_label_);
      return ComputeScore(dataset, index);
    } else {
      cas::Cas<cas::vstring_t> index(approach.type_, {});
      return ComputeScore(dataset, index);
    }
  }
  return 0;
}


template<class VType>
double benchmark::InterleavingExperiment::ComputeScore(
    const Dataset& dataset, cas::Cas<VType>& index) {
  cas::CsvImporter<VType> importer(index, dataset.delim_);
  importer.BulkLoad(dataset.filename_);
  cas::InterleavingScore<VType> score(index);
  return score.Compute();
}


void benchmark::InterleavingExperiment::PrintOutput() {
  std::cout << std::endl;
  std::cout << "dataset,datatype";
  for (auto& approach : approaches_) {
    std::cout << "," << approach.name_;
  }
  std::cout << std::endl;

  for (size_t i = 0; i < results_.size(); ++i) {
    const auto& result = results_[i];
    std::cout << datasets_[i].filename_
      << ","
      << datasets_[i].data_type_;
    for (double score : result) {
      std::cout << "," << score;
    }
    std::cout << std::endl;
  }
  std::cout << std::endl;
}
