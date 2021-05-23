#ifndef BENCHMARK_RUNTIME_EXPERIMENT_H_
#define BENCHMARK_RUNTIME_EXPERIMENT_H_

#include "cas/index.hpp"
#include "cas/cas.hpp"


namespace benchmark {


template<class VType>
class RuntimeExperiment {
private:
  const std::string dataset_filename_;
  const char dataset_delim_;
  const std::vector<std::tuple<double,VType,VType>>& query_values_;
  const std::vector<std::tuple<std::string,std::string>>& query_paths_;
  std::vector<cas::QueryStats> results_;

public:

  RuntimeExperiment(
    const std::string dataset_filename,
    const char dataset_delim,
    const std::vector<std::tuple<double,VType,VType>>& query_values,
    const std::vector<std::tuple<std::string,std::string>>& query_paths)
    : dataset_filename_(dataset_filename)
    , dataset_delim_(dataset_delim)
    , query_values_(query_values)
    , query_paths_(query_paths)
  { }

  void RunIndex(cas::Index<VType>& index, int nr_repetitions);

  void RunAllIndexes();

private:

  void PopulateIndex(cas::Index<VType>& index);

  void PrintOutput();
};


}; // namespace benchmark


#endif // BENCHMARK_RUNTIME_EXPERIMENT_H_
