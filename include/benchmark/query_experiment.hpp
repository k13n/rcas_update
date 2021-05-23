#ifndef BENCHMARK_QUERY_EXPERIMENT_H_
#define BENCHMARK_QUERY_EXPERIMENT_H_

#include "cas/index.hpp"
#include "cas/cas.hpp"
#include "cas/search_key.hpp"


namespace benchmark {


template<class VType>
class QueryExperiment {
public:
  struct Approach {
    cas::IndexType type_;
    std::string name_;
    bool use_surrogate_;
    size_t max_depth_;
    size_t bytes_per_label_;
  };

private:
  const std::string dataset_filename_;
  const char dataset_delim_;
  const std::vector<Approach> approaches_;
  std::vector<cas::SearchKey<VType>> queries_;
  std::vector<cas::QueryStats> results_;

public:
  QueryExperiment(
      const std::string dataset_filename,
      const char dataset_delim,
      const std::vector<Approach> approaches,
      std::vector<cas::SearchKey<VType>> queries
  );

  void Run();

  void RunIndex(cas::Index<VType>& index, int nr_repetitions);

  void PopulateIndex(cas::Index<VType>& index);

  void PrintOutput();

private:
  cas::Index<VType>* CreateIndex(Approach approach);
};


}; // namespace benchmark


#endif // BENCHMARK_QUERY_EXPERIMENT_H_
