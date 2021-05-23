#include "cas/interleaved_key.hpp"
#include <iostream>
#include <cassert>


void cas::InterleavedKey::Dump() const {
  std::cout << "did_:   " << did_ << std::endl;
  std::cout << "bytes_: ";
  for (const InterleavedByte& byte : bytes_) {
    char type;
    switch (byte.type_) {
      case cas::Path:  type = 'p'; break;
      case cas::Value: type = 'v'; break;
      default: assert(false);
    }
    printf("0x%02X%c ", (unsigned char) byte.byte_, type);
  }
  std::cout << std::endl;
}
