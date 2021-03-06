#ifndef CAS_BULK_LOAD_H_
#define CAS_BULK_LOAD_H_

#include "cas/binary_key.hpp"
#include "cas/node.hpp"
#include <map>
#include <limits>
#include <deque>

namespace cas {


const size_t DoesNotExist = std::numeric_limits<std::size_t>::max();

class BulkLoad {
  std::deque<cas::BinaryKey>& keys_;
  cas::NodeType root_split_;

public:
  BulkLoad(std::deque<cas::BinaryKey>& keys,
      cas::NodeType root_split = cas::NodeType::Value);

  cas::Node* Execute();

  void BuildPrefix(
      cas::Node* node,
      cas::BinaryKey& key,
      size_t dp, size_t dv,
      size_t dp_new, size_t dv_new);

private:

  /**
   * dp: position of discriminative path byte
   * dv: position of discriminative value byte
   */
  cas::Node* Construct(
      const std::vector<size_t>& indexes,
      size_t dp, size_t dv, cas::NodeType split_type);

  size_t DiscriminativeByte(
      const std::vector<size_t>& indexes,
      cas::NodeType attribute, size_t lower_bound);

  std::map<uint8_t, std::vector<size_t>> Partition(
      const std::vector<size_t>& indexes,
      cas::NodeType attribute, size_t disc_byte);
};


}; // namespace cas

#endif // CAS_BULK_LOAD_H_
