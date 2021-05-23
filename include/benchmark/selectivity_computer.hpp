#ifndef BENCHMARK_SELECTIVITY_COMPUTER_H_
#define BENCHMARK_SELECTIVITY_COMPUTER_H_

#include "cas/index.hpp"
#include "cas/cas.hpp"


namespace benchmark {


template<class VType>
class SelectivityComputer {
public:
  struct Result {
    size_t nr_matches_;
    double selectivity_;
    double runtime_ms_;
  };

private:
  cas::Cas<VType> index_;

public:
  SelectivityComputer(
      const std::string dataset_filename,
      const char dataset_delim
  );

  void Compute(std::string query_path);

  void Compute(VType low, VType high);

  void Compute(std::string query_path, VType low, VType high);

  void Compute(cas::SearchKey<VType>& key);

private:
  void Selectivity(cas::SearchKey<VType>& key, Result& result);

  void SetAllValues(cas::SearchKey<VType>& skey);

  void SetAllPaths(cas::SearchKey<VType>& skey);
};


}; // namespace benchmark


#endif // BENCHMARK_SELECTIVITY_COMPUTER_H_
