#include "cas/cas_seq.hpp"
#include "cas/key_encoder.hpp"
#include "cas/key_decoder.hpp"
#include "cas/path_matcher.hpp"
#include "cas/utils.hpp"
#include "cas/search_key.hpp"
#include <iostream>
#include <chrono>


template<class VType>
cas::CasSeq<VType>::CasSeq(const std::vector<std::string>& query_path) :
  cas::Index<VType>(query_path) {
}


template<class VType>
cas::CasSeq<VType>::~CasSeq() {
}


template<class VType>
cas::QueryStats cas::CasSeq<VType>::Insert(
    cas::Key<VType>& key,
    cas::UpdateType insertTypeMain,
    cas::UpdateType insertTypeAux,
    cas::InsertTarget insert_target) {
  cas::KeyEncoder<VType> encoder;
  data_.push_back(encoder.Encode(key));
}


template<class VType>
uint64_t cas::CasSeq<VType>::BulkLoad(std::deque<cas::Key<VType>>& keys) {
  const auto& t_start = std::chrono::high_resolution_clock::now();
  for (auto& key : keys) {
    Insert(key, cas::UpdateType::StrictSlow, cas::UpdateType::StrictSlow);
  }
  const auto& t_end = std::chrono::high_resolution_clock::now();
  return std::chrono::duration_cast<std::chrono::microseconds>(t_end-t_start).count();
}


template<class VType>
const cas::QueryStats cas::CasSeq<VType>::Query(cas::SearchKey<VType>& skey,
    cas::BinaryKeyEmitter emitter) {
  cas::QueryStats stats;
  const auto& t_start = std::chrono::high_resolution_clock::now();

  cas::KeyEncoder<VType> encoder;
  cas::BinarySK bskey = encoder.Encode(skey);

  for (const auto& key : data_) {
    // Possible improvement: evaluate path predicate only if the value
    // predicate evaluates to true. The reason I did not implement this
    // is because of the experiments. With this implemented, the runtime
    // of the sequential scan is not constant, which looks weird and
    // unexpected in the experiments. The reason it is not constant is
    // because the higher the selectivity of the value predicate the more
    // often the path predicate has to be evaluated
    bool match_val = MatchesValue(bskey, key);
    bool match_pat = MatchesPath(bskey, key);

    if (match_val && match_pat) {
      ++stats.nr_matches_;
      emitter(key.path_, key.value_, key.did_);
    }
  }

  const auto& t_end = std::chrono::high_resolution_clock::now();
  stats.runtime_mus_ =
    std::chrono::duration_cast<std::chrono::microseconds>(t_end-t_start).count();

  return stats;
}

template<class VType>
const cas::QueryStats cas::CasSeq<VType>::Query(cas::SearchKey<VType>& skey,
    cas::Emitter<VType> emitter) {
  cas::KeyDecoder<VType> decoder;
  return Query(skey, [&](
        const std::vector<uint8_t>& buffer_path,
        const std::vector<uint8_t>& buffer_value,
        cas::did_t did) -> void {
    emitter(decoder.Decode(buffer_path, buffer_value, did));
  });
}


template<class VType>
const cas::QueryStats cas::CasSeq<VType>::QueryRuntime(
    cas::SearchKey<VType>& skey) {
  return Query(skey, [&](
        const std::vector<uint8_t>& /*buffer_path*/,
        const std::vector<uint8_t>& /*buffer_value*/,
        cas::did_t /*did*/) -> void {
  });
}


template<class VType>
bool cas::CasSeq<VType>::MatchesValue(const cas::BinarySK& skey,
    const cas::BinaryKey& key) {
  int c1 = Utils::Memcmp(&skey.low_[0], skey.low_.size(), &key.value_[0], key.value_.size());
  int c2 = Utils::Memcmp(&key.value_[0], key.value_.size(), &skey.high_[0], skey.high_.size());
  return c1 <= 0 && c2 <= 0;
}


template<class VType>
bool cas::CasSeq<VType>::MatchesPath(const cas::BinarySK& skey,
    const cas::BinaryKey& key) {
  cas::PathMatcher pm;
  return pm.MatchPath(key.path_, skey.path_);
}


template<class VType>
void cas::CasSeq<VType>::Describe() {
  cas::IndexStats stats = Stats();
  std::cout << "CasSeq<" << cas::Utils::TypeToString<VType>() << ">" << std::endl;
  std::cout << "Size (keys):  " << stats.nr_keys_ << std::endl;
  std::cout << "Size (bytes): " << stats.size_bytes_ << std::endl;
  std::cout << std::endl;
}


template<class VType>
const cas::IndexStats cas::CasSeq<VType>::Stats() {
  cas::IndexStats stats;
  stats.nr_keys_ = data_.size();
  stats.size_bytes_ = 0;
  for (const auto& key : data_) {
    stats.size_bytes_ += key.path_.size() * sizeof(uint8_t);
    stats.size_bytes_ += key.value_.size() * sizeof(uint8_t);
    stats.size_bytes_ += sizeof(cas::did_t);
  }
  return stats;
}


// explicit instantiations to separate header from implementation
template class cas::CasSeq<cas::vint32_t>;
template class cas::CasSeq<cas::vint64_t>;
template class cas::CasSeq<cas::vstring_t>;
