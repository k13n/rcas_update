#ifndef CAS_KEY_H_
#define CAS_KEY_H_

#include "cas/types.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>


namespace cas {


template<class VType>
struct Key {
  VType value_;
  path_t path_;
  did_t did_;

  Key();

  Key(VType value, path_t path, did_t did);

  void Dump() const;
};

template<class VType>
bool operator==(const Key<VType>& lhs, const Key<VType>& rhs) {
  return lhs.value_ == rhs.value_ &&
    lhs.path_ == rhs.path_ &&
    lhs.did_ == rhs.did_;
}


} // namespace cas

#endif // CAS_KEY_H_
