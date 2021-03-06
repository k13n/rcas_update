#include "benchmark/skew_experiment.hpp"

#include "cas/cas.hpp"
#include "cas/cas_seq.hpp"
#include "cas/key.hpp"
#include "cas/search_key.hpp"
#include "cas/csv_importer.hpp"

#include <iostream>
#include <chrono>


template<class VType>
benchmark::SkewExperiment<VType>::SkewExperiment(
      const std::string query_path,
      const std::vector<Approach> approaches,
      const std::vector<Dataset> datasets,
      const char dataset_delim)
  : query_path_(query_path)
  , approaches_(approaches)
  , datasets_(datasets)
  , dataset_delim_(dataset_delim)
{
}


template<class VType>
void benchmark::SkewExperiment<VType>::Run() {
  int nr_repetitions = 100;
  for (const auto& dataset : datasets_) {
    std::cout << "dataset: " << dataset.filename_ << std::endl;
    for (const auto& approach : approaches_) {
      cas::Index<VType>* index = CreateIndex(approach);
      PopulateIndex(*index, dataset.filename_);
      RunIndex(*index, nr_repetitions, dataset);
      delete index;
    }
  }
  PrintOutput();
}


template<class VType>
void benchmark::SkewExperiment<VType>::RunIndex(
    cas::Index<VType>& index, int nr_repetitions, const Dataset& dataset) {
  index.Describe();
  std::cout << std::endl;

  std::vector<cas::QueryStats> stats;
  for (int i = 0; i < nr_repetitions; ++i) {
    cas::SearchKey<VType> skey;
    skey.low_  = dataset.low_;
    skey.high_ = dataset.high_;
    skey.path_ = { query_path_ };
    stats.push_back(index.QueryRuntime(skey));
  }
  results_.push_back(cas::QueryStats::Avg(stats));
  std::cout << std::endl;
  results_[results_.size()-1].Dump();
  std::cout << std::endl;
  std::cout << std::endl;
}


template<class VType>
void benchmark::SkewExperiment<VType>::PopulateIndex(
    cas::Index<VType>& index, std::string filename) {
  cas::CsvImporter<VType> importer(index, dataset_delim_);
  importer.BulkLoad(filename);
}


template<class VType>
void benchmark::SkewExperiment<VType>::PrintOutput() {
  std::cout << "cardinality,skew";
  for (const auto& approach : approaches_) {
    std::cout << "," << approach.name_;
  }
  std::cout << std::endl;

  for (size_t row = 0; row < datasets_.size(); ++row) {
    const auto& dataset = datasets_[row];
    std::cout << dataset.cardinality_ << "," << dataset.skew_;
    for (size_t col = 0; col < approaches_.size(); ++col) {
      const auto& result = results_[col + row*approaches_.size()];
      double runtime_ms = result.runtime_mus_ / 1000.0;
      std::cout << "," << runtime_ms;
    }
    std::cout << std::endl;
  }
  std::cout << std::endl;
  std::cout << std::endl;
}


template<class VType>
cas::Index<VType>* benchmark::SkewExperiment<VType>::CreateIndex(
    Approach approach) {
  cas::Index<VType>* index = nullptr;
  switch (approach.type_) {
    case cas::IndexType::TwoDimensional:
    case cas::IndexType::PathValue:
    case cas::IndexType::ValuePath:
    case cas::IndexType::ByteInterleaving:
    case cas::IndexType::LevelInterleaving:
    case cas::IndexType::ZOrder:
      if (approach.use_surrogate_) {
        index = new cas::Cas<VType>(approach.type_, {},
           approach.max_depth_, approach.bytes_per_label_);
      } else {
        index = new cas::Cas<VType>(approach.type_, {});
      }
      break;
    case cas::IndexType::Xml:
      index = nullptr;
      break;
    case cas::IndexType::Seq:
      index = new cas::CasSeq<VType>({});
      break;
  }
  return index;
}


// explicit instantiations to separate header from implementation
template class benchmark::SkewExperiment<cas::vint32_t>;
template class benchmark::SkewExperiment<cas::vint64_t>;
template class benchmark::SkewExperiment<cas::vstring_t>;
