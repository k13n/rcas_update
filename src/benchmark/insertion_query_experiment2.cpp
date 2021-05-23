#include "benchmark/insertion_query_experiment2.hpp"
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


template<class VType>
benchmark::InsertionQueryExperiment2<VType>::InsertionQueryExperiment2(
      const std::string dataset_filename_,
      const char dataset_delim,
      const std::vector<cas::InsertMethod>& insert_methods,
      std::vector<cas::SearchKey<VType>> queries,
      double percent_bulkload,
      const std::string perf_datafile
      )
  : dataset_filename_(dataset_filename_)
  , dataset_delim_(dataset_delim)
  , insert_methods_(insert_methods)
  , queries_(queries)
  , percent_bulkload_(percent_bulkload)
  , perf_datafile_(perf_datafile)
{
}


template<class VType>
void benchmark::InsertionQueryExperiment2<VType>::Run() {
  std::cout << "InsertionQuery experiment: " << std::endl;
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
void benchmark::InsertionQueryExperiment2<VType>::RunIndex(
    cas::Cas<VType>& index,
    const cas::InsertMethod& insert_method,
    int nr_repetitions) {

  cas::CsvImporter<VType> importer(index, dataset_delim_);

  std::deque<cas::Key<VType>> keys_all;
  std::deque<cas::Key<VType>> keys_to_bulkload;
  std::deque<cas::Key<VType>> keys_to_insert;

  std::ifstream infile(dataset_filename_);
  std::string line;
  while (std::getline(infile, line)) {
    keys_all.push_back(importer.ProcessLine(line));
  }

  size_t nr_keys_to_bulkload = static_cast<size_t>(percent_bulkload_ * keys_all.size());
  size_t nr_keys_to_insert   = keys_all.size() - nr_keys_to_bulkload;
  while (keys_to_bulkload.size() < nr_keys_to_bulkload) {
    keys_to_bulkload.push_back(keys_all.front());
    keys_all.pop_front();
  }
  while (!keys_all.empty()) {
    keys_to_insert.push_back(keys_all.front());
    keys_all.pop_front();
  }

  std::cout << "keys bulk-loaded (percent): " << percent_bulkload_ << "\n";
  std::cout << "keys bulk-loaded: " << nr_keys_to_bulkload << "\n";
  std::cout << "keys inserted:    " << nr_keys_to_insert << "\n";
  std::cout << "\n\n";

  // bulk-load fraction of the index
  index.BulkLoad(keys_to_bulkload);
  // point insertions for the remaining fraction
  std::vector<cas::QueryStats> stats_insertion_time;
  while (!keys_to_insert.empty()) {
    const auto retval = index.Insert(keys_to_insert.front(),
        insert_method.main_insert_type_,
        insert_method.aux_insert_type_,
        insert_method.target_
    );
    stats_insertion_time.push_back(retval);
    keys_to_insert.pop_front();
  }
  results_insertion_time.push_back(cas::QueryStats::Avg(stats_insertion_time));

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
void benchmark::InsertionQueryExperiment2<VType>::PrintOutput() {
  /// Print of the average insert time in the main and aux index
  std::cout<<"Average insert time in the Main and Aux index:"<<std::endl;
  for (size_t col = 0; col < insert_methods_.size(); ++col) {
    const auto& result = results_insertion_time[col];
    double runtime_main_ms = result.runtime_main_mus_ / 1000.0;
    double runtime_aux_ms = result.runtime_aux_mus_ / 1000.0;
    std::cout << "-Main index: " << runtime_main_ms << std::endl;
    std::cout << "-Aux index: " << runtime_aux_ms << std::endl;
  }
  std::cout << std::endl;

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
    std::cout << "nr_matches_" << result.nr_matches_<< std::endl;
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
template class benchmark::InsertionQueryExperiment2<cas::vint32_t>;
template class benchmark::InsertionQueryExperiment2<cas::vint64_t>;
template class benchmark::InsertionQueryExperiment2<cas::vstring_t>;
