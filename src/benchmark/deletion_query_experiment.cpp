#include "benchmark/deletion_query_experiment.hpp"
#include "benchmark/runtime_experiment.hpp"
#include "benchmark/perf_wrapper.hpp"
#include "cas/cas.hpp"
#include "cas/cas_seq.hpp"
#include "cas/key.hpp"
#include "cas/search_key.hpp"
#include "cas/csv_importer.hpp"
#include <iostream>
#include <chrono>
#include <fstream>
#include <random>


template<class VType>
benchmark::DeletionQueryExperiment<VType>::DeletionQueryExperiment(
      const std::string dataset_filename_,
      const char dataset_delim,
      const std::vector<cas::InsertMethod>& insert_methods,
      std::vector<cas::SearchKey<VType>> queries,
      double percent_query,
      double percent_bulkload,
      const std::string perf_datafile
      )
  : dataset_filename_(dataset_filename_)
  , dataset_delim_(dataset_delim)
  , insert_methods_(insert_methods)
  , queries_(queries)
  , percent_query_(percent_query)
  , percent_bulkload_(percent_bulkload)
  , perf_datafile_(perf_datafile)
{
}


template<class VType>
void benchmark::DeletionQueryExperiment<VType>::Run() {
  std::cout << "DeletionQuery experiment: " << std::endl;
  std::cout << std::endl;

  /* int nr_repetitions = 1000; */
  int nr_repetitions = 100;
  for (const auto& approach : insert_methods_) {
    cas::Cas<VType> index{cas::IndexType::TwoDimensional, {}};
    RunIndex(index, approach, nr_repetitions);

    std::cout<<"Main index number of keys: " << index.root_->nr_keys_<<std::endl;
    if (index.auxiliary_index_ != nullptr) {
      std::cout<<"Auxiliary index number of keys: " <<
        index.auxiliary_index_->nr_keys_ <<std::endl;
    }
    std::cout<<std::endl;
  }
  PrintOutput();
}


template<class VType>
void benchmark::DeletionQueryExperiment<VType>::RunIndex(
    cas::Cas<VType>& index,
    const cas::InsertMethod& insert_method,
    int nr_repetitions) {

  cas::CsvImporter<VType> importer(index, dataset_delim_);

  std::deque<cas::Key<VType>> keys_all;
  std::deque<cas::Key<VType>> keys_to_bulkload;
  std::deque<cas::Key<VType>> keys_to_insert;
  std::deque<cas::Key<VType>> keys_to_delete;

  std::ifstream infile(dataset_filename_);
  std::string line;
  while (std::getline(infile, line)) {
    keys_all.push_back(importer.ProcessLine(line));
  }

  size_t nr_keys_to_query    = static_cast<size_t>(percent_query_ * keys_all.size());
  size_t nr_total_keys       = static_cast<size_t>(percent_bulkload_ * keys_all.size());
  size_t nr_keys_to_bulkload = nr_keys_to_query;
  size_t nr_keys_to_insert   = nr_total_keys - nr_keys_to_query;
  size_t nr_keys_to_delete   = nr_keys_to_insert;

  while (keys_to_bulkload.size() < nr_keys_to_bulkload) {
    keys_to_bulkload.push_back(keys_all.front());
    keys_all.pop_front();
  }
  while (keys_to_insert.size() < nr_keys_to_insert) {
    keys_to_insert.push_back(keys_all.front());
    keys_all.pop_front();
  }
  keys_all.clear();
  keys_all.resize(0);

  size_t keys_from_bulk = 0;
  size_t keys_from_insert = 0;
  std::mt19937_64 rng;
  std::uniform_real_distribution<double> unif{0, 1};
  double fraction_bulk = nr_keys_to_bulkload / static_cast<double>(nr_total_keys);
  while (keys_to_delete.size() < nr_keys_to_delete) {
    auto& key = unif(rng) <= fraction_bulk
      ? keys_to_bulkload[keys_from_bulk++]
      : keys_to_insert[keys_from_insert++];
    keys_to_delete.push_back(key);
  }

  std::cout << "keys to bulk-load (percent): " << percent_bulkload_ << "\n";
  std::cout << "keys total:        " << nr_total_keys << "\n";
  std::cout << "keys to bulk-load: " << nr_keys_to_bulkload << "\n";
  std::cout << "keys to insert:    " << nr_keys_to_insert << "\n";
  std::cout << "keys to delete:    " << nr_keys_to_insert << "\n";
  std::cout << "\n\n";

  // bulk-load fraction of the index
  index.BulkLoad(keys_to_bulkload);

  // perform point insertions
  for (auto& key : keys_to_insert) {
    index.Insert(key,
        insert_method.main_insert_type_,
        insert_method.aux_insert_type_,
        insert_method.target_
    );
  }

  // perform point deletions
  while (!keys_to_delete.empty()) {
    bool success = index.Delete(keys_to_delete.front(), insert_method.main_insert_type_);
    if (!success) {
      throw std::runtime_error{"could not delete a key"};
    }
    keys_to_delete.pop_front();
  }

  index.Describe();
  std::cout << std::endl;


  std::vector<std::vector<cas::QueryStats>> tmp;
  for (size_t i = 0; i < queries_.size(); ++i) {
    tmp.push_back({});
    tmp.back().reserve(nr_repetitions);
  }

  benchmark::PerfWrapper::profile(perf_datafile_, [&](){
    // perform queries one after another and then repeat
    // to avoid caching effects
    for (int i = 0; i < nr_repetitions; ++i) {
      int query_nr = 0;
      for (auto& skey : queries_) {
        cas::QueryStats statistic = index.QueryRuntime(skey);
        tmp[query_nr++].push_back(statistic);
      }
    }
  });

  for (size_t i = 0; i < queries_.size(); ++i) {
    results_.push_back(cas::QueryStats::Avg(tmp[i]));
  }
}


template<class VType>
void benchmark::DeletionQueryExperiment<VType>::PrintOutput() {
  for (size_t row = 0; row < queries_.size(); ++row) {
    std::cout << "q" << row << ":" << std::endl;
    for (size_t col = 0; col < insert_methods_.size(); ++col) {
      const auto& result = results_[row + col*queries_.size()];
      double runtime_ms = result.runtime_mus_ / 1000.0;
      double runtime_main_ms = result.runtime_main_mus_ / 1000.0;
      double runtime_aux_ms = result.runtime_aux_mus_ / 1000.0;
      std::cout << "-runtime_ms: " << runtime_ms << std::endl;
      std::cout << "-runtime_main_ms: " << runtime_main_ms << std::endl;
      std::cout << "-runtime_aux_ms: " << runtime_aux_ms << std::endl;
      std::cout << "Number of matches: " << results_[row + col*queries_.size()].nr_matches_<< std::endl;
      std::cout << std::endl;
    }
    std::cout << std::endl;
  }


  auto print_stats = [](const cas::QueryStats& result) -> void {
    double runtime_ms = result.runtime_mus_ / 1000.0;
    double runtime_main_ms = result.runtime_main_mus_ / 1000.0;
    double runtime_aux_ms = result.runtime_aux_mus_ / 1000.0;
    std::cout << "runtime_ms: " << runtime_ms << std::endl;
    std::cout << "-runtime_main_ms: " << runtime_main_ms << std::endl;
    std::cout << "-runtime_aux_ms: " << runtime_aux_ms << std::endl;
    std::cout << "nr_matches_: " << result.nr_matches_<< std::endl;
    std::cout << "read_path_nodes_: "<< result.read_path_nodes_<< std::endl;
    std::cout << "read_value_nodes_: "<< result.read_value_nodes_ << std::endl;
    std::cout << "read_nodes_: "<< result.read_path_nodes_ + result.read_value_nodes_ << std::endl;
  };

  for (size_t row = 0; row < queries_.size(); ++row) {
    std::cout << "q" << row << ":\n";
    for (size_t col = 0; col < insert_methods_.size(); ++col) {
      const auto& result = results_[row + col*queries_.size()];
      print_stats(result);
      std::cout << "\n";
    }
    std::cout << "\n";
  }


  std::cout << "\nAverage:\n";
  print_stats(cas::QueryStats::Avg(results_));
  std::cout << std::endl;
  std::cout << std::endl;
}

// explicit instantiations to separate header from implementation
template class benchmark::DeletionQueryExperiment<cas::vint32_t>;
template class benchmark::DeletionQueryExperiment<cas::vint64_t>;
template class benchmark::DeletionQueryExperiment<cas::vstring_t>;
