#ifndef BENCHMARK_SCALABILITY_EXPERIMENT_H_
#define BENCHMARK_SCALABILITY_EXPERIMENT_H_

#include "cas/index.hpp"
#include "cas/cas.hpp"


namespace benchmark {


class ScalabilityExperiment {
public:
  struct Approach {
    cas::IndexType type_;
    std::string name_;
    bool use_surrogate_;
    size_t max_depth_;
    size_t bytes_per_label_;
  };

  struct Dataset {
    size_t size_;
    std::string filename_;
  };

private:
  std::vector<Approach> approaches_;
  const std::vector<Dataset> datasets_;
  const char dataset_delim_;
  std::vector<cas::IndexStats> results_;
  std::vector<uint64_t> load_times_;

public:
  ScalabilityExperiment(
      const std::vector<Dataset> datasets,
      const char dataset_delim);

  void Run();

  void RunIndex(cas::Index<cas::vint32_t>& index, const Dataset& dataset);

  uint64_t PopulateIndex(cas::Index<cas::vint32_t>& index, std::string filename);

  void PrintOutput();

  void PrintTableSpace();

  void PrintTableSpacePerKey();

  void PrintTableTime();

  cas::Index<cas::vint32_t>* CreateIndex(Approach approach);
};


}; // namespace benchmark


#endif // BENCHMARK_SCALABILITY_EXPERIMENT_H_
