#ifndef CAS_BINARY_KEY_H_
#define CAS_BINARY_KEY_H_

#include "cas/types.hpp"
#include "cas/node_type.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>


namespace cas {


struct BinaryKey {
  std::vector<uint8_t> path_;
  std::vector<uint8_t> value_;
  did_t did_ = 0;

  const std::vector<uint8_t>& Get(cas::NodeType attribute);

  void Dump() const;
};


} // namespace cas

#endif // CAS_BINARY_KEY_H_
