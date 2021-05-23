#include "cas/interleaver.hpp"
#include "cas/key_encoding.hpp"
#include <iostream>
#include <algorithm>


cas::InterleavedKey cas::Interleaver::ZOrder(const cas::BinaryKey& bkey) {
  cas::InterleavedKey ikey;
  ikey.bytes_.reserve(bkey.path_.size() + bkey.value_.size());
  ikey.did_ = bkey.did_;

  int p_run_len = bkey.path_.size() / bkey.value_.size();
  size_t p_pos = 0;
  size_t v_pos = 0;

  while (p_pos < bkey.path_.size()
      && v_pos < bkey.value_.size()) {
    ikey.bytes_.push_back({ .byte_ = bkey.value_[v_pos], .type_ = Value });
    ++v_pos;
    for (int i = 0; i < p_run_len; ++i) {
      ikey.bytes_.push_back({ .byte_ = bkey.path_[p_pos],  .type_ = Path });
      ++p_pos;
    }
  }

  while (v_pos < bkey.value_.size()) {
    ikey.bytes_.push_back({ .byte_ = bkey.value_[v_pos],  .type_ = Value });
    ++v_pos;
  }
  while (p_pos < bkey.path_.size()) {
    ikey.bytes_.push_back({ .byte_ = bkey.path_[p_pos],  .type_ = Path });
    ++p_pos;
  }

  return ikey;
}


cas::InterleavedKey cas::Interleaver::ByteByte(const cas::BinaryKey& bkey) {
  size_t nr_bytes = bkey.path_.size() + bkey.value_.size();
  size_t min_size = std::min(bkey.path_.size(), bkey.value_.size());

  cas::InterleavedKey ikey;
  ikey.bytes_.reserve(nr_bytes);
  ikey.did_ = bkey.did_;

  size_t i = 0;
  for (; i < 2*min_size; ++i) {
    size_t pos = i / 2;
    if (i % 2 == 0) {
      ikey.bytes_.push_back({ .byte_ = bkey.path_[pos],  .type_ = Path });
    } else {
      ikey.bytes_.push_back({ .byte_ = bkey.value_[pos], .type_ = Value });
    }
  }
  for (size_t pos = min_size; i < nr_bytes; ++i, ++pos) {
    if (pos < bkey.path_.size()) {
      ikey.bytes_.push_back({ .byte_ = bkey.path_[pos],  .type_ = Path });
    } else {
      ikey.bytes_.push_back({ .byte_ = bkey.value_[pos], .type_ = Value });
    }
  }

  return ikey;
}


cas::InterleavedKey cas::Interleaver::LevelByte(const cas::BinaryKey& bkey) {
  size_t nr_bytes = bkey.path_.size() + bkey.value_.size();

  cas::InterleavedKey ikey;
  ikey.bytes_.reserve(nr_bytes);
  ikey.did_ = bkey.did_;

  size_t p_pos = 0;
  size_t v_pos = 0;

  auto take_path_label = [&]() -> void {
    if (p_pos >= bkey.path_.size()) {
      return;
    }
    do {
      ikey.bytes_.push_back({ .byte_ = bkey.path_[p_pos], .type_ = Path });
      ++p_pos;
    } while (p_pos < bkey.path_.size() && bkey.path_[p_pos] != cas::kPathSep);
    if (p_pos < bkey.path_.size()) {
      ikey.bytes_.push_back({ .byte_ = bkey.path_[p_pos], .type_ = Path });
      ++p_pos;
    }
  };
  auto take_value_byte = [&]() -> void {
    if (v_pos >= bkey.value_.size()) {
      return;
    }
    ikey.bytes_.push_back({ .byte_ = bkey.value_[v_pos], .type_ = Value });
    ++v_pos;
  };

  while (v_pos + p_pos < nr_bytes) {
    take_path_label();
    take_value_byte();
  }

  return ikey;
}


cas::InterleavedKey cas::Interleaver::PathValue(const cas::BinaryKey& bkey) {
  cas::InterleavedKey ikey;
  ikey.bytes_.reserve(bkey.path_.size() + bkey.value_.size());
  ikey.did_ = bkey.did_;
  for (size_t i = 0; i < bkey.path_.size(); ++i) {
    ikey.bytes_.push_back({ .byte_ = bkey.path_[i], .type_ = Path });
  }
  for (size_t i = 0; i < bkey.value_.size(); ++i) {
    ikey.bytes_.push_back({ .byte_ = bkey.value_[i], .type_ = Value });
  }
  return ikey;
}


cas::InterleavedKey cas::Interleaver::ValuePath(const cas::BinaryKey& bkey) {
  cas::InterleavedKey ikey;
  ikey.bytes_.reserve(bkey.path_.size() + bkey.value_.size());
  ikey.did_ = bkey.did_;
  for (size_t i = 0; i < bkey.value_.size(); ++i) {
    ikey.bytes_.push_back({ .byte_ = bkey.value_[i], .type_ = Value });
  }
  for (size_t i = 0; i < bkey.path_.size(); ++i) {
    ikey.bytes_.push_back({ .byte_ = bkey.path_[i], .type_ = Path });
  }
  return ikey;
}
