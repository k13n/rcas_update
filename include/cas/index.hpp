#ifndef CAS_INDEX_H_
#define CAS_INDEX_H_

#include "cas/key.hpp"
#include "cas/search_key.hpp"
#include "cas/query_stats.hpp"
#include "cas/index_stats.hpp"
#include "cas/update_type.hpp"

#include <functional>
#include <deque>

namespace cas {


template<class VType>
using Emitter = std::function<void(const Key<VType>&)>;

using DidEmitter = std::function<void(did_t)>;

using BinaryEmitter = std::function<void(const std::vector<uint8_t>&)>;

using BinaryKeyEmitter = std::function<void(
    const std::vector<uint8_t>& buffer_path,
    const std::vector<uint8_t>& buffer_value,
    did_t did)>;


template<class VType>
class Index {
protected:
  const std::vector<std::string> query_path_;

public:
  Index(const std::vector<std::string>& query_path) :
    query_path_(query_path)
  {}

  virtual ~Index();

  virtual cas::QueryStats Insert(
      Key<VType>& key,
      cas::UpdateType insertTypeMain,
      cas::UpdateType insertTypeAux,
      cas::InsertTarget insert_target = cas::InsertTarget::MainAuxiliary
  ) = 0;

  virtual uint64_t BulkLoad(std::deque<Key<VType>>& keys) = 0;

  virtual const QueryStats Query(SearchKey<VType>& key, Emitter<VType> emitter) = 0;

  virtual const QueryStats QueryRuntime(SearchKey<VType>& key) = 0;

  virtual const IndexStats Stats() = 0;

  const QueryStats QueryVerbose(SearchKey<VType>& key);

  const std::vector<std::string>& GetQueryPath() const;

  virtual void Describe() = 0;
};


} // namespace cas

#endif // CAS_INDEX_H_
