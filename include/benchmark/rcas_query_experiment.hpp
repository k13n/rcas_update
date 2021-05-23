#ifndef BENCHMARK_RCAS_QUERY_EXPERIMENT_H_
#define BENCHMARK_RCAS_QUERY_EXPERIMENT_H_

#include "cas/index.hpp"
#include "cas/cas.hpp"
#include "cas/search_key.hpp"


namespace benchmark {


template<class VType>
class RcasQueryExperiment {
private:
  const std::string dataset_filename_;
  const char dataset_delim_;
  std::vector<cas::SearchKey<VType>> queries_;
  std::vector<cas::QueryStats> results_;

public:
  RcasQueryExperiment(
      const std::string dataset_filename,
      const char dataset_delim,
      std::vector<cas::SearchKey<VType>> queries
  );

  void Run();

  void RunIndex(cas::Index<VType>& index, int nr_repetitions);

  void PopulateIndex(cas::Index<VType>& index);

  void PrintOutput();
};


}; // namespace benchmark


#endif // BENCHMARK_RCAS_QUERY_EXPERIMENT_H_
