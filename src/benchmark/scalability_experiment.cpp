#include "benchmark/scalability_experiment.hpp"

#include "cas/cas.hpp"
#include "cas/cas_seq.hpp"
#include "cas/key.hpp"
#include "cas/search_key.hpp"
#include "cas/csv_importer.hpp"

#include <iostream>
#include <chrono>


benchmark::ScalabilityExperiment::ScalabilityExperiment(
      const std::vector<Dataset> datasets,
      const char dataset_delim)
  : datasets_(datasets)
  , dataset_delim_(dataset_delim)
{
  approaches_ = {
    { cas::IndexType::TwoDimensional,    "dy",  false, 0, 0 },
    /* { cas::IndexType::ZOrder,            "zo",  true,  8, 3 }, */
    { cas::IndexType::ZOrder,            "zo",  true, 21, 3 },
    { cas::IndexType::ByteInterleaving,  "bw",  false, 0, 0 },
    { cas::IndexType::LevelInterleaving, "lw",  false, 0, 0 },
    { cas::IndexType::PathValue,         "pv",  false, 0, 0 },
    { cas::IndexType::ValuePath,         "vp",  false, 0, 0 },
  };
}


void benchmark::ScalabilityExperiment::Run() {
  for (const auto& dataset : datasets_) {
    std::cout << "dataset: " << dataset.filename_ << std::endl;
    for (const auto& approach : approaches_) {
      cas::Index<cas::vint32_t>* index = CreateIndex(approach);
      RunIndex(*index, dataset);
      delete index;
    }
  }
  PrintOutput();
}


void benchmark::ScalabilityExperiment::RunIndex(
    cas::Index<cas::vint32_t>& index, const Dataset& dataset) {
  uint64_t load_time = PopulateIndex(index, dataset.filename_);
  index.Describe();
  std::cout << std::endl;
  cas::IndexStats stats = index.Stats();
  results_.push_back(stats);
  load_times_.push_back(load_time);
}


uint64_t benchmark::ScalabilityExperiment::PopulateIndex(
    cas::Index<cas::vint32_t>& index, std::string filename) {
  cas::CsvImporter<cas::vint32_t> importer(index, dataset_delim_);
  return importer.BulkLoad(filename);
}


void benchmark::ScalabilityExperiment::PrintOutput() {
  std::cout << "Space Consumption:" << std::endl << std::endl;
  PrintTableSpace();
  std::cout << std::endl;
  std::cout << std::endl;
  std::cout << std::endl;
  std::cout << "Bytes per key:" << std::endl << std::endl;
  PrintTableSpacePerKey();
  std::cout << std::endl;
  std::cout << std::endl;
  std::cout << std::endl;
  std::cout << "Bulk Loading Time:" << std::endl << std::endl;
  PrintTableTime();
  std::cout << std::endl;
}


void benchmark::ScalabilityExperiment::PrintTableSpace() {
  std::cout << "size";
  for (const auto& approach :  approaches_) {
    std::cout << "," << approach.name_;
  }
  std::cout << std::endl;
  for (size_t row = 0; row < datasets_.size(); ++row) {
    std::cout << datasets_[row].size_;
    for (size_t col = 0; col < approaches_.size(); ++col) {
      int pos = row * approaches_.size() + col;
      std::cout << "," << results_[pos].size_bytes_;
    }
    std::cout << std::endl;
  }
}


void benchmark::ScalabilityExperiment::PrintTableSpacePerKey() {
  std::cout << "size";
  for (const auto& approach :  approaches_) {
    std::cout << "," << approach.name_;
  }
  std::cout << std::endl;
  for (size_t row = 0; row < datasets_.size(); ++row) {
    std::cout << datasets_[row].size_;
    for (size_t col = 0; col < approaches_.size(); ++col) {
      int pos = row * approaches_.size() + col;
      size_t index_size = results_[pos].size_bytes_;
      double bytes_per_key = index_size / (double) datasets_[row].size_;
      std::cout << "," << bytes_per_key;
    }
    std::cout << std::endl;
  }
}


void benchmark::ScalabilityExperiment::PrintTableTime() {
  std::cout << "size";
  for (const auto& approach :  approaches_) {
    std::cout << "," << approach.name_;
  }
  std::cout << std::endl;
  for (size_t row = 0; row < datasets_.size(); ++row) {
    std::cout << datasets_[row].size_;
    for (size_t col = 0; col < approaches_.size(); ++col) {
      int pos = row * approaches_.size() + col;
      double runtime_ms = load_times_[pos] / 1000.0;
      std::cout << "," << runtime_ms;
    }
    std::cout << std::endl;
  }
}


cas::Index<cas::vint32_t>* benchmark::ScalabilityExperiment::CreateIndex(
    Approach approach) {
  using VType = cas::vint32_t;
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
