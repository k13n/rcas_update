#ifndef CAS_CAS_SEQ_H_
#define CAS_CAS_SEQ_H_

#include "cas/types.hpp"
#include "cas/index.hpp"
#include "cas/key.hpp"
#include "cas/binary_key.hpp"
#include "cas/search_key.hpp"
#include "cas/query_stats.hpp"
#include <vector>
#include <deque>
#include <tuple>

namespace cas {


template<class VType>
class CasSeq : public Index<VType> {
  std::deque<BinaryKey> data_;

public:
  CasSeq(const std::vector<std::string>& query_path);

  ~CasSeq();

  cas::QueryStats Insert(Key<VType>& key,
      cas::InsertType insertTypeMain,
      cas::InsertType insertTypeAux,
      cas::InsertTarget insert_target = cas::InsertTarget::MainAuxiliary
  );

  uint64_t BulkLoad(std::deque<Key<VType>>& keys);

  const QueryStats Query(SearchKey<VType>& key,
      BinaryKeyEmitter emitter);

  const QueryStats Query(SearchKey<VType>& skey,
      Emitter<VType> emitter);

  const QueryStats QueryRuntime(SearchKey<VType>& key);

  void Describe();

  void Dump();

  void DumpConcise();

  size_t Size();

  const std::deque<BinaryKey>& Data() const {
    return data_;
  }

  const IndexStats Stats();

private:
  bool MatchesValue(const BinarySK& skey, const BinaryKey& key);

  bool MatchesPath(const BinarySK& skey, const BinaryKey& key);

  std::string CasType();
};


} // namespace CAS_CAS_SEQ_H_

#endif // CAS_CAS_H_
