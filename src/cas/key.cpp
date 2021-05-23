#include "cas/key.hpp"

#include <iostream>


template<class VType>
cas::Key<VType>::Key()
{}


template<class VType>
cas::Key<VType>::Key(VType value, path_t path, did_t did) :
    value_(value),
    path_(path),
    did_(did)
{ }


template<class VType>
void cas::Key<VType>::Dump() const {
  std::cout << "Value: " << value_ << std::endl;
  std::cout << "Path: ";
  for (auto& v : path_) {
    std::cout << "/" << v;
  }
  std::cout << std::endl;
  std::cout << "DID: " << did_ << std::endl;
}


// explicit instantiations to separate header from implementation
template struct cas::Key<cas::vint32_t>;
template struct cas::Key<cas::vint64_t>;
template struct cas::Key<cas::vstring_t>;
