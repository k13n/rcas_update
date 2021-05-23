#include "cas/index.hpp"

#include <iostream>

template<class VType>
cas::Index<VType>::~Index() {
}


template<class VType>
const cas::QueryStats cas::Index<VType>::QueryVerbose(cas::SearchKey<VType>& key) {
  key.Dump();
  std::cout << std::endl;
  return Query(key, [](const cas::Key<VType>& match) -> void {
    std::cout << "Match:" << std::endl;
    match.Dump();
    std::cout << std::endl;
  });
}


template<class VType>
const std::vector<std::string>& cas::Index<VType>::GetQueryPath() const {
  return query_path_;
}


// explicit instantiations to separate header from implementation
template class cas::Index<cas::vint32_t>;
template class cas::Index<cas::vint64_t>;
template class cas::Index<cas::vstring_t>;
