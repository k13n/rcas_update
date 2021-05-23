#include "benchmark/robustness_experiment.hpp"
#include "benchmark/runtime_experiment.hpp"

#include "cas/cas.hpp"
#include "cas/cas_seq.hpp"
#include "cas/key.hpp"
#include "cas/search_key.hpp"
#include "cas/csv_importer.hpp"

#include <iostream>
#include <chrono>


template<class VType>
benchmark::RobustnessExperiment<VType>::RobustnessExperiment(
      const std::string dataset_filename,
      const char dataset_delim,
      const std::vector<Approach> approaches,
      const std::vector<std::tuple<VPred, PPred>> predicates)
  : dataset_filename_(dataset_filename)
  , dataset_delim_(dataset_delim)
  , approaches_(approaches)
  , predicates_(predicates)
{
}


template<class VType>
void benchmark::RobustnessExperiment<VType>::Run() {
  int nr_repetitions = 100;
  for (const auto& approach : approaches_) {
    cas::Index<VType>* index = CreateIndex(approach);
    PopulateIndex(*index);
    RunIndex(*index, nr_repetitions);
    delete index;
  }
  PrintOutput();
}


template<class VType>
void benchmark::RobustnessExperiment<VType>::RunIndex(
    cas::Index<VType>& index, int nr_repetitions) {
  index.Describe();
  std::cout << std::endl;

  for (const auto& predicate : predicates_) {
    const auto& v_pred = std::get<0>(predicate);
    const auto& p_pred = std::get<1>(predicate);
    std::vector<cas::QueryStats> stats;
    for (int i = 0; i < nr_repetitions; ++i) {
      cas::SearchKey<VType> skey;
      skey.low_  = v_pred.low_;
      skey.high_ = v_pred.high_;
      skey.path_ = { p_pred.path_ };
      stats.push_back(index.QueryRuntime(skey));
    }
    results_.push_back(cas::QueryStats::Avg(stats));
  }
}


template<class VType>
void benchmark::RobustnessExperiment<VType>::PopulateIndex(
    cas::Index<VType>& index) {
  cas::CsvImporter<VType> importer(index, dataset_delim_);
  importer.BulkLoad(dataset_filename_);
}


template<class VType>
cas::Index<VType>* benchmark::RobustnessExperiment<VType>::CreateIndex(
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


template<class VType>
void benchmark::RobustnessExperiment<VType>::PrintOutput() {
  std::cout << "q_pred" << ",v_pred";
  for (const auto& approach : approaches_) {
    std::cout << "," << approach.name_;
  }
  std::cout << std::endl;

  for (size_t row = 0; row < predicates_.size(); ++row) {
    const auto& v_pred = std::get<0>(predicates_[row]);
    const auto& p_pred = std::get<1>(predicates_[row]);

    std::cout << p_pred.label_ << "," << v_pred.label_;
    for (size_t col = 0; col < approaches_.size(); ++col) {
      const auto& result = results_[row + col*predicates_.size()];
      double runtime_ms = result.runtime_mus_ / 1000.0;
      std::cout << "," << runtime_ms;
    }
    std::cout << std::endl;
  }
  std::cout << std::endl;
  std::cout << std::endl;
}


// explicit instantiations to separate header from implementation
template class benchmark::RobustnessExperiment<cas::vint32_t>;
template class benchmark::RobustnessExperiment<cas::vint64_t>;
template class benchmark::RobustnessExperiment<cas::vstring_t>;
