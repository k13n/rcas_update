#include "benchmark/selectivity_computer.hpp"
#include "cas/csv_importer.hpp"

#include <iostream>
#include <cstdint>


template<class VType>
benchmark::SelectivityComputer<VType>::SelectivityComputer(
      const std::string dataset_filename,
      const char dataset_delim)
  : index_(cas::IndexType::TwoDimensional, {})
{
  cas::CsvImporter<VType> importer(index_, dataset_delim);
  importer.BulkLoad(dataset_filename);
  index_.Describe();
}


template<class VType>
void benchmark::SelectivityComputer<VType>::Compute(
    std::string query_path) {
  cas::SearchKey<VType> skey;
  skey.path_ = { query_path };
  SetAllValues(skey);
  Compute(skey);
}


template<class VType>
void benchmark::SelectivityComputer<VType>::Compute(
    VType low, VType high) {
  cas::SearchKey<VType> skey;
  skey.path_ = { "^" };
  skey.low_  = low;
  skey.high_ = high;
  Compute(skey);
}


template<class VType>
void benchmark::SelectivityComputer<VType>::Compute(
    std::string query_path,
    VType low, VType high) {
  cas::SearchKey<VType> skey;
  skey.path_ = { query_path };
  skey.low_  = low;
  skey.high_ = high;
  Compute(skey);
}


template<class VType>
void benchmark::SelectivityComputer<VType>::Compute(
    cas::SearchKey<VType>& skey) {
  skey.DumpConcise();
  Result r;
  Selectivity(skey, r);
  std::cout << "selectivity:     " << r.nr_matches_ <<
    " (" << r.selectivity_ << "), " << r.runtime_ms_ << "ms" << std::endl;
  {
    cas::SearchKey<VType> s2 = skey;
    SetAllValues(s2);
    Selectivity(s2, r);
    std::cout << "selectivity (p): " << r.nr_matches_ <<
      " (" << r.selectivity_ << "), " << r.runtime_ms_ << "ms" << std::endl;
  }
  {
    cas::SearchKey<VType> s3 = skey;
    SetAllPaths(s3);
    Selectivity(s3, r);
    std::cout << "selectivity (v): " << r.nr_matches_ <<
      " (" << r.selectivity_ << "), " << r.runtime_ms_ << "ms" << std::endl;
  }
  std::cout << std::endl;
}


template<class VType>
void benchmark::SelectivityComputer<VType>::Selectivity(
    cas::SearchKey<VType>& skey, Result& r) {
  cas::QueryStats stats = index_.QueryRuntime(skey);
  r.nr_matches_  = stats.nr_matches_;
  r.selectivity_ = static_cast<double>(stats.nr_matches_) / index_.nr_keys_;
  r.runtime_ms_  = stats.runtime_mus_ / 1000.0;
}


template<class VType>
void benchmark::SelectivityComputer<VType>::SetAllPaths(
    cas::SearchKey<VType>& skey) {
  skey.path_ = { "^" };
}


template<>
void benchmark::SelectivityComputer<cas::vint32_t>::SetAllValues(
    cas::SearchKey<cas::vint32_t>& skey) {
  skey.low_  = cas::VINT32_MIN;
  skey.high_ = cas::VINT32_MAX;
}
template<>
void benchmark::SelectivityComputer<cas::vint64_t>::SetAllValues(
    cas::SearchKey<cas::vint64_t>& skey) {
  skey.low_  = cas::VINT64_MIN;
  skey.high_ = cas::VINT64_MAX;
}
template<>
void benchmark::SelectivityComputer<cas::vstring_t>::SetAllValues(
    cas::SearchKey<cas::vstring_t>& skey) {
  skey.low_  = cas::VSTRING_MIN;
  skey.high_ = cas::VSTRING_MAX;
}


// explicit instantiations to separate header from implementation
template class benchmark::SelectivityComputer<cas::vint32_t>;
template class benchmark::SelectivityComputer<cas::vint64_t>;
template class benchmark::SelectivityComputer<cas::vstring_t>;
