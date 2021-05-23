#include "benchmark/space_experiment.hpp"
#include "benchmark/runtime_experiment.hpp"

#include "cas/cas.hpp"
#include "cas/cas_seq.hpp"
#include "cas/key.hpp"
#include "cas/search_key.hpp"
#include "cas/csv_importer.hpp"

#include <iostream>
#include <chrono>


template<class VType>
benchmark::SpaceExperiment<VType>::SpaceExperiment(
      const std::string dataset_filename,
      const char dataset_delim,
      const std::vector<Approach> approaches)
  : dataset_filename_(dataset_filename)
  , dataset_delim_(dataset_delim)
  , approaches_(approaches)
{
}


template<class VType>
void benchmark::SpaceExperiment<VType>::Run() {
  std::cout << "dataset: " << dataset_filename_ << std::endl;
  std::cout << std::endl;

  {
    cas::CasSeq<VType> seq_idx({});
    PopulateIndex(seq_idx);
    seq_idx.Describe();
    cas::IndexStats stats = seq_idx.Stats();
    bytes_per_key_ = stats.size_bytes_ / static_cast<double>(stats.nr_keys_);
    nr_keys_ = stats.nr_keys_;
  }

  for (const auto& approach : approaches_) {
    cas::Index<VType>* index = CreateIndex(approach);
    PopulateIndex(*index);
    RunIndex(*index);
    delete index;
  }
  PrintOutput();
}


template<class VType>
void benchmark::SpaceExperiment<VType>::RunIndex(
    cas::Index<VType>& index) {
  index.Describe();
  results_.push_back(index.Stats());
}


template<class VType>
void benchmark::SpaceExperiment<VType>::PopulateIndex(
    cas::Index<VType>& index) {
  cas::CsvImporter<VType> importer(index, dataset_delim_);
  importer.BulkLoad(dataset_filename_);
}


template<class VType>
cas::Index<VType>* benchmark::SpaceExperiment<VType>::CreateIndex(
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
void benchmark::SpaceExperiment<VType>::PrintOutput() {
  std::cout << "bytes_per_key_: " << bytes_per_key_ << std::endl;
  std::cout << std::endl;

  std::cout << "dataset";
  for (const auto& approach : approaches_) {
    std::cout << "," << approach.name_;
  }
  std::cout << std::endl;
  std::cout << "x";
  for (size_t i = 0; i < approaches_.size(); ++i) {
    const auto& result = results_[i];
    std::cout << "," << result.size_bytes_;
  }
  std::cout << std::endl;
  std::cout << std::endl;

  std::cout << "dataset";
  for (const auto& approach : approaches_) {
    std::cout << "," << approach.name_;
  }
  std::cout << std::endl;
  std::cout << "x";
  for (size_t i = 0; i < approaches_.size(); ++i) {
    const auto& result = results_[i];
    std::cout << "," << (result.size_bytes_ / static_cast<double>(nr_keys_));
  }
  std::cout << std::endl;
  std::cout << std::endl;
}


// explicit instantiations to separate header from implementation
template class benchmark::SpaceExperiment<cas::vint32_t>;
template class benchmark::SpaceExperiment<cas::vint64_t>;
template class benchmark::SpaceExperiment<cas::vstring_t>;
