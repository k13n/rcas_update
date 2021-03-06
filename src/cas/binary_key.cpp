#include "cas/binary_key.hpp"

#include "cas/utils.hpp"
#include <iostream>


const std::vector<uint8_t>& cas::BinaryKey::Get(cas::NodeType attribute) {
  switch (attribute) {
  case cas::NodeType::Path:
    return path_;
  case cas::NodeType::Value:
    return value_;
  default:
    throw std::runtime_error("invalid");
  }
}

void cas::BinaryKey::Dump() const {
  printf("Path  (%2lu): ", path_.size());
  cas::Utils::DumpHexValues(path_);
  std::cout << std::endl;
  printf("Value (%2lu): ", value_.size());
  cas::Utils::DumpHexValues(value_);
  std::cout << std::endl;
  std::cout << "DID:        " <<  did_;
  std::cout << std::endl;
}
