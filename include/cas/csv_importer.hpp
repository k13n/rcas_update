#ifndef CAS_CSV_IMPORTER_H_
#define CAS_CSV_IMPORTER_H_

#include "cas/index.hpp"
#include "cas/key.hpp"
#include "cas/update_type.hpp"

namespace cas {


template<class VType>
class CsvImporter {
  Index<VType>& index_;
  did_t highest_did_ = 0;
  char delimiter_;

public:
  CsvImporter(Index<VType>& index, const char delimiter_ = ' ');

  void Load(std::string filename, cas::UpdateType insertTypeMain, cas::UpdateType insertTypeAux);

  uint64_t BulkLoad(std::string filename);

  const cas::Key<VType> ProcessLine(const std::string& line);

private:
  VType ParseValue(std::string& value);
};


}; // namespace cas

#endif // CAS_CSV_IMPORTER_H_


