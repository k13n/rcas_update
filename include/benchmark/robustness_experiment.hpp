#ifndef BENCHMARK_ROBUSTNESS_EXPERIMENT_H_
#define BENCHMARK_ROBUSTNESS_EXPERIMENT_H_

#include "cas/index.hpp"
#include "cas/cas.hpp"


namespace benchmark {


template<class VType>
class RobustnessExperiment {
public:
  struct Approach {
    cas::IndexType type_;
    std::string name_;
    bool use_surrogate_;
    size_t max_depth_;
    size_t bytes_per_label_;
  };

  struct VPred {
    std::string label_;
    double selectivity_;
    VType low_;
    VType high_;
  };

  struct PPred {
    std::string label_;
    double selectivity_;
    std::string path_;
  };

private:
  const std::string dataset_filename_;
  const char dataset_delim_;
  const std::vector<Approach> approaches_;
  const std::vector<std::tuple<VPred, PPred>> predicates_;
  std::vector<cas::QueryStats> results_;

public:
  RobustnessExperiment(
      const std::string dataset_filename,
      const char dataset_delim,
      const std::vector<Approach> approaches,
      const std::vector<std::tuple<VPred, PPred>> predicates
  );

  void Run();

  void RunIndex(cas::Index<VType>& index, int nr_repetitions);

  void PopulateIndex(cas::Index<VType>& index);

  void PrintOutput();

private:
  cas::Index<VType>* CreateIndex(Approach approach);
};


}; // namespace benchmark


#endif // BENCHMARK_ROBUSTNESS_EXPERIMENT_H_
