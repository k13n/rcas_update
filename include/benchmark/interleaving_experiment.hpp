#ifndef BENCHMARK_INTERLEAVING_EXPERIMENT_H_
#define BENCHMARK_INTERLEAVING_EXPERIMENT_H_

#include "cas/index.hpp"
#include "cas/cas.hpp"


namespace benchmark {


class InterleavingExperiment {
public:
  struct Dataset {
    std::string filename_;
    std::string data_type_;
    char delim_;
  };

  struct Approach {
    cas::IndexType type_;
    std::string name_;
    bool use_surrogate_;
    size_t max_depth_;
    size_t bytes_per_label_;
  };

private:
  const std::vector<Dataset> datasets_;
  const std::vector<Approach> approaches_;
  std::vector<std::vector<double>> results_;

public:

  InterleavingExperiment(
    const std::vector<Dataset> datasets_,
    const std::vector<Approach> approaches)
    : datasets_(datasets_)
    , approaches_(approaches)
  { }

  void Run();

  double RunIndex(const Dataset& dataset,
      const Approach& approach);

  template<class VType>
  double ComputeScore(const Dataset& dataset,
      cas::Cas<VType>& index);

private:

  void PrintOutput();
};


}; // namespace benchmark


#endif // BENCHMARK_INTERLEAVING_EXPERIMENT_H_
