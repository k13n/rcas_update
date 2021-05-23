#ifndef BENCHMARK_INSERTION_EXPERIMENT_H_
#define BENCHMARK_INSERTION_EXPERIMENT_H_

#include "cas/index.hpp"
#include "cas/cas.hpp"
#include "cas/search_key.hpp"
#include "cas/insert_type.hpp"


namespace benchmark {


template<class VType>
class InsertionExperiment {
public:
  struct Approach {
    cas::IndexType type_;
    std::string name_;
    bool use_surrogate_;
    size_t max_depth_;
    size_t bytes_per_label_;
  };

private:
  const std::string initial_dataset_filename_;
  const std::string insertion_dataset_filename_;
  const char dataset_delim_;
  const std::vector<Approach> approaches_;
//  std::vector<cas::SearchKey<VType>> queries_;
  std::vector<cas::QueryStats> results_;

public:
  InsertionExperiment(
      const std::string initial_dataset_filename_,
      const std::string insertion_dataset_filename_,
      const char dataset_delim,
      const std::vector<Approach> approaches
  );

  void Run();

  void RunIndex(cas::Index<VType>& index);

  void PopulateIndex(cas::Index<VType>& index);

  void PrintOutput();

private:
  cas::Index<VType>* CreateIndex(Approach approach);
};


}; // namespace benchmark


#endif // BENCHMARK_INSERTION_EXPERIMENT_H_
