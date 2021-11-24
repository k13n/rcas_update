#include "benchmark/query_experiment.hpp"
#include "benchmark/option_parser.hpp"

#include "cas/cas.hpp"
#include "cas/cas_seq.hpp"

#include <benchmark/deletion_experiment.hpp>
#include <iostream>


void Benchmark(const benchmark::Config& config) {
  using VType = cas::vint64_t;
  using Exp = benchmark::DeletionExperiment<VType>;

  std::vector<cas::InsertMethod> insert_methods = {
    config.insert_method_,
  };

  Exp bm(
      config.input_filename_,
      config.dataset_delim_,
      insert_methods,
      config.percent_bulkload_
  );

  bm.Run();
}


int main(int argc, char** argv) {
  benchmark::Config config = benchmark::option_parser::Parse(argc, argv);
  Benchmark(config);
  return 0;
}
