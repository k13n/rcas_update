#ifndef CAS_INSERTION_HELPER_H_
#define CAS_INSERTION_HELPER_H_

#include "cas/search_key.hpp"
#include <cstdint>
#include <vector>

namespace cas {


class InsertionHelper{
public:
  struct State {
    uint16_t ppos_ = 0;
    uint16_t qpos_ = 0;
    uint16_t desc_ppos_ = 0;
    int16_t  desc_qpos_ = -1;

    void Dump();
  };

   State InsertionMatchPathIncremental(
      const std::vector<uint8_t>& path,
      const cas::BinaryQP& query_path,
      size_t len_path,
      State& state);


  State InsertionMatchPath(
      const std::vector<uint8_t>& path,
      const cas::BinaryQP& query_path);
};


}; // namespace cas

#endif // CAS_INSERTION_HELPER_H_    