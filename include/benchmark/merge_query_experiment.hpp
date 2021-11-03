#ifndef BENCHMARK_INSERTION_QUERY_EXPERIMENT2_H_
#define BENCHMARK_INSERTION_QUERY_EXPERIMENT2_H_

#include "cas/index.hpp"
#include "cas/cas.hpp"
#include "cas/search_key.hpp"
#include "cas/update_type.hpp"
#include <chrono>


namespace benchmark {


template<class VType>
class MergeQueryExperiment {
private:
  const std::string dataset_filename_;
  const char dataset_delim_;
  const std::vector<cas::MergeMethod>& merge_methods_;
  std::vector<cas::SearchKey<VType>> queries_;
  std::vector<cas::QueryStats> results_;
  std::vector<cas::QueryStats> results_insertion_time;
  std::vector<size_t> results_merge_time_;
  double percent_bulkload_;
  const std::string perf_datafile_;

public:
  MergeQueryExperiment(
      const std::string dataset_filename_,
      const char dataset_delim,
      const std::vector<cas::MergeMethod>& merge_methods,
      std::vector<cas::SearchKey<VType>> queries,
      double percent_bulkload_,
      const std::string perf_datafile
  );

  void Run();

  void RunIndex(cas::Cas<VType>& index,
      const cas::MergeMethod& merge_method,
      int nr_repetitions);

  void PrintOutput();
};


}; // namespace benchmark


#endif // BENCHMARK_QUERY_INSERTION_EXPERIMENT2_H_
