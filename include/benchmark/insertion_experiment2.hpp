#ifndef BENCHMARK_INSERTION_EXPERIMENT2_H_
#define BENCHMARK_INSERTION_EXPERIMENT2_H_

#include "cas/index.hpp"
#include "cas/cas.hpp"
#include "cas/search_key.hpp"
#include "cas/insert_type.hpp"


namespace benchmark {


template<class VType>
class InsertionExperiment2 {
private:
  const std::string dataset_filename_;
  const char dataset_delim_;
  const std::vector<cas::InsertMethod>& insert_methods_;
  std::vector<cas::QueryStats> results_;
  double percent_bulkload_;

public:
  InsertionExperiment2(
      const std::string dataset_filename_,
      const char dataset_delim,
      const std::vector<cas::InsertMethod>& insert_methods,
      double percent_bulkload
  );

  void Run();

  void RunIndex(cas::Cas<VType>& index,
      const cas::InsertMethod& insert_method);

  void PrintOutput();
};


}; // namespace benchmark


#endif // BENCHMARK_INSERTION_EXPERIMENT2_H_
