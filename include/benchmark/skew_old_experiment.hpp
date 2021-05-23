#ifndef BENCHMARK_SKEW_OLD_EXPERIMENT_H_
#define BENCHMARK_SKEW_OLD_EXPERIMENT_H_

#include "cas/index.hpp"
#include "cas/cas.hpp"


namespace benchmark {


class SkewOldExperiment {
public:
  struct Dataset {
    size_t cardinality_;
    double skew_;
    std::string filename_;
  };

private:
  const std::vector<Dataset> datasets_;
  const char dataset_delim_;
  const std::string query_path_;
  std::vector<cas::QueryStats> results_;

public:
  SkewOldExperiment(
      const std::vector<Dataset> datasets,
      const char dataset_delim,
      const std::string query_path);

  void Run();

  void RunIndex(cas::Index<cas::vint32_t>& index, int nr_repetitions);

  void PopulateIndex(cas::Index<cas::vint32_t>& index, std::string filename);

  void PrintOutput();
};


}; // namespace benchmark


#endif // BENCHMARK_SKEW_OLD_EXPERIMENT_H_
