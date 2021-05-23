#include "benchmark/query_experiment.hpp"
#include "benchmark/runtime_experiment.hpp"
#include "cas/cas.hpp"
#include "cas/cas_seq.hpp"
#include "cas/key.hpp"
#include "cas/search_key.hpp"
#include "cas/csv_importer.hpp"
#include <iostream>
#include <chrono>


template<class VType>
benchmark::QueryExperiment<VType>::QueryExperiment(
      const std::string dataset_filename,
      const char dataset_delim,
      const std::vector<Approach> approaches,
      std::vector<cas::SearchKey<VType>> queries)
  : dataset_filename_(dataset_filename)
  , dataset_delim_(dataset_delim)
  , approaches_(approaches)
  , queries_(queries)
{
}


template<class VType>
void benchmark::QueryExperiment<VType>::Run() {
  std::cout << "Queries: " << std::endl;
  for (size_t i = 0; i < queries_.size(); ++i) {
    std::cout << "q" << i << ": ";
    queries_[i].DumpConcise();
  }
  std::cout << std::endl;

  int nr_repetitions = 100;
    for (const auto& approach : approaches_) {
    cas::Index<VType>* index = CreateIndex(approach);
    PopulateIndex(*index);
    RunIndex(*index, nr_repetitions);

// To print output index to file
//    std::cout << "\n QueryExperiment index Dump"<<"\n";
//    static_cast<cas::Cas<VType> *>(index)->root_->DumpConciseToFile(0x00, 00, "../datasets/index.txt");

// To print output index to command line
//    std::cout << "\n QueryExperiment index Dump"<<"\n";
//    static_cast<cas::Cas<VType> *>(index)->root_->DumpConcise(0x00,0);


delete index;
  }
  PrintOutput();
}


template<class VType>
void benchmark::QueryExperiment<VType>::RunIndex(cas::Index<VType>& index, int nr_repetitions) {
  index.Describe();
  std::cout << std::endl;

  for (auto& skey : queries_) {
    std::vector<cas::QueryStats> stats;
    for (int i = 0; i < nr_repetitions; ++i) {
      cas::QueryStats statistic = index.QueryRuntime(skey);
      if(i == 99){
        std::cout<<"Read Path Nodes: "<< statistic.read_path_nodes_<< std::endl;
        std::cout<<"Total Value Nodes: "<< statistic.read_value_nodes_ << std::endl;
        std::cout<<"Total Read Nodes: "<< statistic.read_path_nodes_ + statistic.read_value_nodes_ << std::endl;
        std::cout<<std::endl;
      }
      stats.push_back(statistic);
    }
    results_.push_back(cas::QueryStats::Avg(stats));
  }
}


template<class VType>
void benchmark::QueryExperiment<VType>::PopulateIndex(
    cas::Index<VType>& index) {
  cas::CsvImporter<VType> importer(index, dataset_delim_);
  uint64_t bulkload_time = importer.BulkLoad(dataset_filename_);
  std::cout<<"BulkLoad time (milliseconds) = " <<bulkload_time / 1000.0 << std::endl;
}


template<class VType>
cas::Index<VType>* benchmark::QueryExperiment<VType>::CreateIndex(
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
void benchmark::QueryExperiment<VType>::PrintOutput() {
  std::cout << "query";
  for (const auto& approach : approaches_) {
    std::cout << "," << approach.name_;
  }
  std::cout << std::endl;

  for (size_t row = 0; row < queries_.size(); ++row) {
    std::cout << "q" << row;
    for (size_t col = 0; col < approaches_.size(); ++col) {
      const auto& result = results_[row + col*queries_.size()];
      double runtime_ms = result.runtime_mus_ / 1000.0;
      std::cout << "," << runtime_ms;
      std::cout << std::endl;
      std::cout << "Number of matches: " << results_[row + col*queries_.size()].nr_matches_<< std::endl;
    }
    std::cout << std::endl;
  }
  std::cout << std::endl;
  std::cout << std::endl;
}


// explicit instantiations to separate header from implementation
template class benchmark::QueryExperiment<cas::vint32_t>;
template class benchmark::QueryExperiment<cas::vint64_t>;
template class benchmark::QueryExperiment<cas::vstring_t>;
