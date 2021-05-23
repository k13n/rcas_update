#include "benchmark/insertion_experiment.hpp"
#include "cas/cas.hpp"
#include "cas/cas_seq.hpp"
#include "cas/key.hpp"
#include "cas/csv_importer.hpp"
#include <iostream>
#include <chrono>


template<class VType>
benchmark::InsertionExperiment<VType>::InsertionExperiment(
      const std::string initial_dataset_filename_,
      const std::string insertion_dataset_filename_,
      const char dataset_delim,
      const std::vector<Approach> approaches)
  : initial_dataset_filename_(initial_dataset_filename_)
  , insertion_dataset_filename_(insertion_dataset_filename_)
  , dataset_delim_(dataset_delim)
  , approaches_(approaches)
{
}


template<class VType>
void benchmark::InsertionExperiment<VType>::Run() {
   std::cout << "Insertion experiment: " << std::endl;
   std::cout << std::endl;

  for (const auto& approach : approaches_) {
    cas::Index<VType>* index = CreateIndex(approach);
    const auto& t_start = std::chrono::high_resolution_clock::now();

    PopulateIndex(*index);
    RunIndex(*index);

    const auto& t_end = std::chrono::high_resolution_clock::now();
    const auto& runtime_mus_ = std::chrono::duration_cast<std::chrono::microseconds>(t_end-t_start).count();

    std::cout << "Total time, PopulateIndex + RunIndex: " << runtime_mus_ << " micro seconds" << std::endl;

    index->Describe();
    std::cout<<"Main index number of keys: " <<static_cast<cas::Cas<VType> *>(index)->root_->nr_keys_<<std::endl;
    if(static_cast<cas::Cas<VType> *>(index)->auxiliary_index_ != nullptr) std::cout<<"Auxiliary index number of keys: " <<static_cast<cas::Cas<VType> *>(index)->auxiliary_index_->nr_keys_<<std::endl;

    //Merging indexes
//    if(static_cast<cas::Cas<VType> *>(index)->auxiliary_index_ != nullptr) static_cast<cas::Cas<VType> *>(index)->mergeMainAndAuxiliaryIndex();
//    index->Describe();
//    std::cout<<"Main index after merge, number of keys: " <<static_cast<cas::Cas<VType> *>(index)->root_->nr_keys_<<std::endl;

    //To print output index to file
//    std::cout << "\n InsertionQueryExperiment index Dump"<<"\n";
//    static_cast<cas::Cas<VType> *>(index)->root_->DumpConciseToFile(0x00, 00, "../datasets/index.txt");

   delete index;
  }
  PrintOutput();
}

template<class VType>
void benchmark::InsertionExperiment<VType>::RunIndex(cas::Index<VType>& index) {
  index.Describe();
  std::cout << std::endl;

  cas::CsvImporter<VType> importer(index, dataset_delim_);
  std::ifstream infile(insertion_dataset_filename_);
  std::string line;
  std::vector<cas::QueryStats> stats;
  long counter = 0;

  const auto& t_start = std::chrono::high_resolution_clock::now();

  while (std::getline(infile, line)) {
   counter ++;
   cas::Key<VType> key = importer.ProcessLine(line);
   //***In Insert method setting of insertTypes for main and auxiliary index is possible***
   cas::QueryStats single_stats = index.Insert(key, cas::InsertType::StrictSlow, cas::InsertType::StrictSlow);
   if(single_stats.runtime_mus_ != 0) {
       stats.push_back(single_stats);
   }
  }

  const auto& t_end = std::chrono::high_resolution_clock::now();
  const auto& runtime_mus_ = std::chrono::duration_cast<std::chrono::microseconds>(t_end-t_start).count();
  std::cout << "Main and auxiliry index execution time: " << runtime_mus_ << " micro seconds" << std::endl;
  results_.push_back(cas::QueryStats::Avg(stats));
  std::cout <<"RunIndex stats.size: "<<stats.size() <<std::endl;
}


template<class VType>
void benchmark::InsertionExperiment<VType>::PopulateIndex(cas::Index<VType>& index) {
  cas::CsvImporter<VType> importer(index, dataset_delim_);
  importer.BulkLoad(initial_dataset_filename_);
}


template<class VType>
cas::Index<VType>* benchmark::InsertionExperiment<VType>::CreateIndex(Approach approach) {
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
void benchmark::InsertionExperiment<VType>::PrintOutput() {
  std::cout << "Experiment";
  for (const auto& approach : approaches_) {
    std::cout << "," << approach.name_;
  }
  std::cout << std::endl;

    for (size_t col = 0; col < approaches_.size(); ++col) {
      const auto& result = results_[col];
      double runtime_ms = result.runtime_mus_ / 1000.0;
      std::cout << "," << runtime_ms;
    }
    std::cout << std::endl;
  std::cout << std::endl;
  std::cout << std::endl;
}


// explicit instantiations to separate header from implementation
template class benchmark::InsertionExperiment<cas::vint32_t>;
template class benchmark::InsertionExperiment<cas::vint64_t>;
template class benchmark::InsertionExperiment<cas::vstring_t>;