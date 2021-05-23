#ifndef CAS_INTERLEAVED_KEY_H_
#define CAS_INTERLEAVED_KEY_H_

#include "cas/node_type.hpp"
#include "cas/types.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>


namespace cas {


struct InterleavedByte {
  uint8_t byte_;
  NodeType type_;
};

struct InterleavedKey {
  std::vector<InterleavedByte> bytes_;
  did_t did_ = 0;

  void Dump() const;
};


} // namespace cas

#endif // CAS_INTERLEAVED_KEY_H_
