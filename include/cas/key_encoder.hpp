#ifndef CAS_KEY_ENCODER_H_
#define CAS_KEY_ENCODER_H_

#include "cas/key.hpp"
#include "cas/search_key.hpp"
#include "cas/binary_key.hpp"
#include "cas/surrogate.hpp"
#include <cstring>

namespace cas {


template<class VType>
class KeyEncoder {
public:
  BinaryKey Encode(const Key<VType>& key);

  BinaryKey Encode(const Key<VType>& key, Surrogate& surrogate);

  BinarySK Encode(SearchKey<VType>& key);

  BinarySK EncodeInsertKey(SearchKey<VType>& key);

  BinarySK Encode(SearchKey<VType>& key, Surrogate& surrogate);

  void EncodeValue(const Key<VType>& key, BinaryKey& bkey);

private:
  void ReserveSpace(const Key<VType>& key, BinaryKey& bkey);

  void EncodePath(const Key<VType>& key, BinaryKey& bkey);

  void EncodePath(const Key<VType>& key, BinaryKey& bkey,
      Surrogate& surrogate);

  void EncodeValue(const VType& value, std::vector<uint8_t>& buffer);

  size_t ValueSize(const VType& value);

  void EncodeQueryPath(SearchKey<VType>& key, BinarySK& bkey);

  void EncodeInsertKeyPath(SearchKey<VType>& key, BinarySK& bkey);

  void EncodeQueryPath(SearchKey<VType>& key, BinarySK& bkey,
      Surrogate& surrogate);

  static inline void MemCpyToBuffer(std::vector<uint8_t>& buffer, int& offset,
      const void* value, std::size_t size) {
    std::memcpy(&buffer[offset], value, size);
    offset += size;
  }
};


} // namespace cas

#endif // CAS_KEY_ENCODER_H_
