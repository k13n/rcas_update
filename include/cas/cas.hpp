#ifndef CAS_CAS_H_
#define CAS_CAS_H_

#include "cas/index.hpp"
#include "cas/key.hpp"
#include "cas/search_key.hpp"
#include "cas/query_stats.hpp"
#include "cas/binary_key.hpp"
#include "cas/index_type.hpp"
#include "cas/node.hpp"
#include "cas/surrogate.hpp"
#include "cas/insertion_helper.hpp"
#include "cas/insert_type.hpp"
#include <vector>
#include <stack>


namespace cas {


template<class VType>
class Cas : public Index<VType> {
public:
  IndexType index_type_;
  Node *root_ = nullptr;
  size_t nr_keys_ = 0;
  Node *auxiliary_index_ = nullptr; //auxiliary index
  Surrogate surrogate_;
  bool use_surrogate_;

  Cas(IndexType type, const std::vector<std::string>& query_path);

  Cas(IndexType type, const std::vector<std::string>& query_path,
      size_t s_max_depth, size_t s_bytes_per_label);

  ~Cas();

  cas::QueryStats Insert(Key<VType>& key,
      cas::InsertType insertTypeMain,
      cas::InsertType insertTypeAux,
      cas::InsertTarget insert_target = cas::InsertTarget::MainAuxiliary
  );

  void Insert(const BinaryKey& bkey);

  void Delete(const BinaryKey& bkey);

  uint64_t BulkLoad(std::deque<Key<VType>>& keys);

  uint64_t BulkLoad(std::deque<BinaryKey>& keys, cas::NodeType nodeType= cas::NodeType::Value);

  const QueryStats Query(SearchKey<VType>& key,
      BinaryKeyEmitter emitter);

  const QueryStats Query(SearchKey<VType>& key,
      DidEmitter emitter);

  const QueryStats Query(SearchKey<VType>& key,
      Emitter<VType> emitter);

  const QueryStats QueryRuntime(SearchKey<VType>& key);

  void Describe();

  void Dump();

  void DumpConcise();

  void DumpLatex(const std::string& message = "");

  size_t Size();

  const cas::IndexStats Stats();

  void setAuxiliaryIndex(Node *index);

  Node* getAuxiliaryIndex();

  void mergeMainAndAuxiliaryIndex(cas::MergeMethod merge_method);

private:
  void DeleteNodesRecursively(Node *node);

  void DumpLatexRoot();
};

} // namespace cas

#endif // CAS_CAS_H_
