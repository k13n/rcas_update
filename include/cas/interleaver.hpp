#ifndef CAS_INTERLEAVER_H_
#define CAS_INTERLEAVER_H_

#include "cas/binary_key.hpp"
#include "cas/interleaved_key.hpp"


namespace cas {


class Interleaver {
public:
  static InterleavedKey ByteByte(const BinaryKey& bkey);

  static InterleavedKey LevelByte(const BinaryKey& bkey);

  static InterleavedKey PathValue(const BinaryKey& bkey);

  static InterleavedKey ValuePath(const BinaryKey& bkey);

  static InterleavedKey ZOrder(const BinaryKey& bkey);

};


} // namespace cas

#endif // CAS_INTERLEAVER_H_
