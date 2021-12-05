#include "benchmark/deletion_experiment.hpp"
#include "benchmark/runtime_experiment.hpp"
#include "benchmark/perf_wrapper.hpp"
#include "cas/cas.hpp"
#include "cas/cas_seq.hpp"
#include "cas/key.hpp"
#include "cas/key_encoder.hpp"
#include "cas/search_key.hpp"
#include "cas/csv_importer.hpp"
#include <iostream>
#include <chrono>
#include <fstream>
#include <cmath>
#include <random>


template<class VType>
benchmark::DeletionExperiment<VType>::DeletionExperiment(
      const std::string dataset_filename_,
      const char dataset_delim,
      const std::vector<cas::InsertMethod>& insert_methods,
      double percent_bulkload
      )
  : dataset_filename_(dataset_filename_)
  , dataset_delim_(dataset_delim)
  , insert_methods_(insert_methods)
  , percent_bulkload_(percent_bulkload)
{
}


template<class VType>
void benchmark::DeletionExperiment<VType>::Run() {
  std::cout << "Deletion experiment: " << std::endl;
  std::cout << std::endl;

  for (const auto& approach : insert_methods_) {
    cas::Cas<VType> index{cas::IndexType::TwoDimensional, {}};
    RunIndex(index, approach);
    std::cout<<std::endl;
  }
}


template<class VType>
void benchmark::DeletionExperiment<VType>::RunIndex(
    cas::Cas<VType>& index,
    const cas::InsertMethod& insert_method) {

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

  size_t nr_keys_to_bulkload = static_cast<size_t>(percent_bulkload_ * keys_all.size());
  size_t nr_keys_to_insert   = keys_all.size() - nr_keys_to_bulkload;
  size_t nr_keys_to_delete   = nr_keys_to_insert;

  while (keys_to_bulkload.size() < nr_keys_to_bulkload) {
    keys_to_bulkload.push_back(keys_all.front());
    keys_all.pop_front();
  }
  while (!keys_all.empty()) {
    keys_to_insert.push_back(keys_all.front());
    keys_all.pop_front();
  }
  keys_all.clear();
  keys_all.resize(0);

  size_t keys_from_bulk = 0;
  size_t keys_from_insert = 0;
  std::mt19937_64 rng;
  std::uniform_real_distribution<double> unif{0, 1};
  while (keys_to_delete.size() < nr_keys_to_delete) {
    auto& key = unif(rng) <= percent_bulkload_
      ? keys_to_bulkload[keys_from_bulk++]
      : keys_to_insert[keys_from_insert++];
    keys_to_delete.push_back(key);
  }

  std::cout << "keys to bulk-load (percent): " << percent_bulkload_ << "\n";
  std::cout << "keys to bulk-load: " << nr_keys_to_bulkload << "\n";
  std::cout << "keys to insert:    " << nr_keys_to_insert << "\n";
  std::cout << "keys to delete:    " << nr_keys_to_insert << "\n";
  std::cout << "\n\n";

  // bulk-load fraction of the index
  auto runtime = index.BulkLoad(keys_to_bulkload);
  std::cout << "Runtime bulk-loading: " << runtime << std::endl;

  // perform point insertions
  for (auto& key : keys_to_insert) {
    index.Insert(key,
        insert_method.main_insert_type_,
        insert_method.aux_insert_type_,
        insert_method.target_
    );
  }

  // point deletions for the inserted keys
  std::vector<size_t> deletion_times;
  deletion_times.reserve(keys_to_insert.size());

  // mesure deletion runtimes
  auto t1 = std::chrono::high_resolution_clock::now();
  for (auto& key : keys_to_delete) {
    auto start = std::chrono::high_resolution_clock::now();
    bool success = index.Delete(key, insert_method.main_insert_type_);
    if (!success) {
      throw std::runtime_error{"could not delete a key"};
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto runtime = std::chrono::duration_cast<std::chrono::microseconds>(end-start).count();
    deletion_times.push_back(runtime);
  }
  auto t2 = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::microseconds>(t2-t1).count();

  // compute histogram
  const int histogram_size = 15;
  long histogram[histogram_size];
  for (int i = 0; i < histogram_size; ++i) {
    histogram[i] = 0;
  }
  for (auto time : deletion_times) {
    int pos = (time == 0)
      ? 0
      : 1 + static_cast<int>(std::log10(time));
    if (pos >= histogram_size) {
      pos = histogram_size-1;
    }
    ++histogram[pos];
  }

  // compute stats
  double avg = duration / static_cast<double>(nr_keys_to_insert);
  double variance = 0;
  for (auto time : deletion_times) {
    variance += std::pow(static_cast<double>(time) - avg, 2);
  }
  variance /= nr_keys_to_insert;
  double stddev = std::sqrt(variance);

  // print outputc
  std::cout << "Total deletion runtime (mus): " << duration << "\n";
  std::cout << "Average deletion runtime (mus): " << avg << "\n";
  std::cout << "Variance (mus^2): " << variance << "\n";
  std::cout << "Standard deviation (mus): " << stddev << "\n\n";
  std::cout << "Histogram:\n";
  std::cout << "low;high;value;percent\n";
  for (int i = 0; i < histogram_size; ++i) {
    size_t low, high;
    if (i == 0) {
      low  = 0;
      high = 1;
    } else {
      low  = static_cast<size_t>(std::pow(10, i-1));
      high = static_cast<size_t>(std::pow(10, i));
    }
    double percent = histogram[i] / static_cast<double>(nr_keys_to_insert);
    std::cout << low << ";" << high << ";" << histogram[i] << ";" << percent << "\n";
  }
  std::cout << std::endl;
}


// explicit instantiations to separate header from implementation
template class benchmark::DeletionExperiment<cas::vint32_t>;
template class benchmark::DeletionExperiment<cas::vint64_t>;
template class benchmark::DeletionExperiment<cas::vstring_t>;
