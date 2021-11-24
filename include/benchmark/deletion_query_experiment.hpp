#ifndef BENCHMARK_DELETION_QUERY_EXPERIMENT_H_
#define BENCHMARK_DELETION_QUERY_EXPERIMENT_H_

#include "cas/index.hpp"
#include "cas/cas.hpp"
#include "cas/search_key.hpp"
#include "cas/update_type.hpp"


namespace benchmark {


template<class VType>
class DeletionQueryExperiment {
private:
  const std::string dataset_filename_;
  const char dataset_delim_;
  const std::vector<cas::InsertMethod>& insert_methods_;
  std::vector<cas::SearchKey<VType>> queries_;
  std::vector<cas::QueryStats> results_;
  double percent_query_;
  double percent_bulkload_;
  const std::string perf_datafile_;

public:
  DeletionQueryExperiment(
      const std::string dataset_filename_,
      const char dataset_delim,
      const std::vector<cas::InsertMethod>& insert_methods,
      std::vector<cas::SearchKey<VType>> queries,
      double percent_query,
      double percent_bulkload,
      const std::string perf_datafile
  );

  void Run();

  void RunIndex(cas::Cas<VType>& index,
      const cas::InsertMethod& insert_method,
      int nr_repetitions);

  void PrintOutput();
};


}; // namespace benchmark


#endif // BENCHMARK_QUERY_DELETION_EXPERIMENT_H_
