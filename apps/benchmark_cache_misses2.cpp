#include "benchmark/deletion_query_experiment.hpp"
#include "benchmark/option_parser.hpp"
#include "cas/cas.hpp"

#include <iostream>



template<class VType>
cas::SearchKey<VType> Query(std::string path, VType low, VType high) {
  cas::SearchKey<VType> skey;
  skey.path_ = { path };
  skey.low_  = low;
  skey.high_ = high;
  return skey;
}


void Benchmark(const benchmark::Config& config) {
  using VType = cas::vint64_t;
  using Exp = benchmark::DeletionQueryExperiment<VType>;

  std::vector<cas::InsertMethod> insert_methods = {
    config.insert_method_,
  };

  std::vector<cas::SearchKey<VType>> queries = {
    Query<VType>("/usr/include^", 5000, cas::VINT64_MAX),
    Query<VType>("/usr/include^", 3000, 4000),
    Query<VType>("/usr/lib^", 0, 1000),
    Query<VType>("/usr/share^Makefile", 1000, 2000),
    Query<VType>("/usr/share/doc^README", 4000, 5000),
    Query<VType>("/etc^", 5000, cas::VINT64_MAX),
  };

  Exp bm(
    config.input_filename_,
    config.dataset_delim_,
    insert_methods,
    queries,
    config.percent_query_,
    config.percent_bulkload_,
    config.perf_datafile_
  );

  bm.Run();
}


int main(int argc, char** argv) {
  benchmark::Config config = benchmark::option_parser::Parse(argc, argv);
  Benchmark(config);
  return 0;
}
