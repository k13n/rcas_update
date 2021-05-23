#include "benchmark/insertion_query_experiment.hpp"
#include "benchmark/runtime_experiment.hpp"
#include "cas/cas.hpp"
#include "cas/cas_seq.hpp"
#include "cas/key.hpp"
#include "cas/search_key.hpp"
#include "cas/csv_importer.hpp"
#include <iostream>
#include <chrono>
#include <fstream>


template<class VType>
benchmark::InsertionQueryExperiment<VType>::InsertionQueryExperiment(
      const std::string initial_dataset_filename_,
      const std::string insertion_dataset_filename_,
      const char dataset_delim,
      const std::vector<Approach> approaches,
      std::vector<cas::SearchKey<VType>> queries)
  : initial_dataset_filename_(initial_dataset_filename_)
  , insertion_dataset_filename_(insertion_dataset_filename_)
  , dataset_delim_(dataset_delim)
  , approaches_(approaches)
  , queries_(queries)
{
}


template<class VType>
void benchmark::InsertionQueryExperiment<VType>::Run() {
    std::cout << "InsertionQuery experiment: " << std::endl;
    std::cout << std::endl;

    int nr_repetitions = 100;
    for (const auto& approach : approaches_) {
    cas::Index<VType>* index = CreateIndex(approach);
    PopulateIndex(*index);
    RunIndex(*index, nr_repetitions);

    std::cout<<"Main index number of keys: " <<static_cast<cas::Cas<VType> *>(index)->root_->nr_keys_<<std::endl;
    if (static_cast<cas::Cas<VType> *>(index)->auxiliary_index_ != nullptr) {
      std::cout<<"Auxiliary index number of keys: " <<
        static_cast<cas::Cas<VType>*>(index)->auxiliary_index_->nr_keys_ <<std::endl;
    }

    std::cout<<std::endl;

    // To print output index to file
//    std::cout << "\n InsertionQueryExperiment index Dump"<<"\n";
//    static_cast<cas::Cas<VType> *>(index)->root_->DumpConciseToFile(0x00, 00, "../datasets/index.txt");

    // To print output index to command line
//    std::cout << "\n InsertionQueryExperiment index Dump"<<"\n";
//    static_cast<cas::Cas<VType> *>(index)->root_->DumpConcise(0x00,0);

    delete index;
    }
  PrintOutput();
}


template<class VType>
void benchmark::InsertionQueryExperiment<VType>::RunIndex(cas::Index<VType>& index, int nr_repetitions) {
  index.Describe();
  std::cout << std::endl;

  cas::CsvImporter<VType> importer(index, dataset_delim_);
  std::ifstream infile(insertion_dataset_filename_);
  std::string line;
  std::vector<cas::QueryStats> stats_insertion_time;

  while (std::getline(infile, line)) {
    cas::Key<VType> key = importer.ProcessLine(line);

    //***In Insert method setting of insertTypes for main and auxiliary index is possible***
//    stats_insertion_time.push_back(index.Insert(key, cas::InsertType::StrictSlow, cas::InsertType::StrictSlow));
    stats_insertion_time.push_back(index.Insert(key, cas::InsertType::LazyFast, cas::InsertType::LazyFast));
  }

  results_insertion_time.push_back(cas::QueryStats::Avg(stats_insertion_time));

  std::cout<<"Main index number of keys after insert: " <<static_cast<cas::Cas<VType> &>(index).root_->nr_keys_<<std::endl;
  if (static_cast<cas::Cas<VType> &>(index).auxiliary_index_ != nullptr) {
    std::cout<<"Auxiliary index number of keys after insert: " <<
      static_cast<cas::Cas<VType> &>(index).auxiliary_index_->nr_keys_ <<std::endl;
  }

  //Merging indexes
  const auto& t_start = std::chrono::high_resolution_clock::now();

  /* static_cast<cas::Cas<VType>&>(index).mergeMainAndAuxiliaryIndex(); */

  const auto& t_end = std::chrono::high_resolution_clock::now();
  int64_t merge_runtime_mus_ = std::chrono::duration_cast<std::chrono::microseconds>(t_end-t_start).count();
  double merge_runtime_ms_ = (merge_runtime_mus_) / 1000.0;
  std::cout << "Time to merge indexes (milliseconds): " << merge_runtime_ms_ << std::endl;

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
void benchmark::InsertionQueryExperiment<VType>::PopulateIndex(
    cas::Index<VType>& index) {
  cas::CsvImporter<VType> importer(index, dataset_delim_);
  importer.BulkLoad(initial_dataset_filename_);
}


template<class VType>
cas::Index<VType>* benchmark::InsertionQueryExperiment<VType>::CreateIndex(
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
void benchmark::InsertionQueryExperiment<VType>::PrintOutput() {
  std::cout << "query";
  for (const auto& approach : approaches_) {
    std::cout << "," << approach.name_;
  }
  std::cout << std::endl;

  /// Print of the average insert time in the main and aux index
  std::cout<<"Average insert time in the Main and Aux index:"<<std::endl;
  for (size_t col = 0; col < approaches_.size(); ++col) {
      const auto& result = results_insertion_time[col];
        double runtime_main_ms = result.runtime_main_mus_ / 1000.0;
        double runtime_aux_ms = result.runtime_aux_mus_ / 1000.0;
        std::cout << "-Main index: " << runtime_main_ms << std::endl;
        std::cout << "-Aux index: " << runtime_aux_ms << std::endl;
  }
    std::cout << std::endl;

  for (size_t row = 0; row < queries_.size(); ++row) {
    std::cout << "q" << row << ":" << std::endl;
    for (size_t col = 0; col < approaches_.size(); ++col) {
      const auto& result = results_[row + col*queries_.size()];
      double runtime_ms = result.runtime_mus_ / 1000.0;
      double runtime_main_ms = result.runtime_main_mus_ / 1000.0;
      double runtime_aux_ms = result.runtime_aux_mus_ / 1000.0;
      std::cout << "-runtime_ms:" << runtime_ms << std::endl;
      std::cout << "-runtime_main_ms:" << runtime_main_ms << std::endl;
      std::cout << "-runtime_aux_ms:" << runtime_aux_ms << std::endl;
      std::cout << std::endl;
      std::cout << "Number of matches: " << results_[row + col*queries_.size()].nr_matches_<< std::endl;
    }
    std::cout << std::endl;
  }
  std::cout << std::endl;
  std::cout << std::endl;
}

// explicit instantiations to separate header from implementation
template class benchmark::InsertionQueryExperiment<cas::vint32_t>;
template class benchmark::InsertionQueryExperiment<cas::vint64_t>;
template class benchmark::InsertionQueryExperiment<cas::vstring_t>;
