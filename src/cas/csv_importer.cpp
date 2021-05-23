#include "cas/csv_importer.hpp"
#include "cas/key.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <deque>


template<class VType>
cas::CsvImporter<VType>::CsvImporter(cas::Index<VType>& index, char delimiter)
    : index_(index),
    delimiter_(delimiter) {
}


template<class VType>
void cas::CsvImporter<VType>::Load(std::string filename, cas::InsertType insertTypeMain, cas::InsertType insertTypeAux) {
  highest_did_ = 0;
  std::ifstream infile(filename);
  std::string line;
  while (std::getline(infile, line)) {
    cas::Key<VType> key = ProcessLine(line);
    index_.Insert(key, insertTypeMain, insertTypeAux);
  }
}


template<class VType>
uint64_t cas::CsvImporter<VType>::BulkLoad(std::string filename) {
  std::deque<cas::Key<VType>> keys;
  highest_did_ = 0;
  std::ifstream infile(filename);
  std::string line;

  while (std::getline(infile, line)) {
    cas::Key<VType> key = ProcessLine(line);
    keys.push_back(key);
  }
  return index_.BulkLoad(keys);
}


template<class VType>
const cas::Key<VType> cas::CsvImporter<VType>::ProcessLine(const std::string& line) {
  cas::Key<VType> key;

  std::stringstream line_stream(line);
  std::string path, value, did;
  std::getline(line_stream, path,  delimiter_);
  std::getline(line_stream, value, delimiter_);
  std::getline(line_stream, did,   delimiter_);

  std::stringstream path_stream(path);
  std::string label;
  while (std::getline(path_stream, label, '/')) {
    if (!label.empty()) {
      key.path_.push_back(label);
    }
  }
  key.value_ = ParseValue(value);
  key.did_   = did.empty() ?
                highest_did_ + 1 :
                std::stoull(did);
  highest_did_ = key.did_;

  return key;
}


template<>
cas::vint32_t cas::CsvImporter<cas::vint32_t>::ParseValue(std::string& value) {
  return std::stoi(value);
}
template<>
cas::vint64_t cas::CsvImporter<cas::vint64_t>::ParseValue(std::string& value) {
  return std::stoll(value);
}
template<>
std::string cas::CsvImporter<std::string>::ParseValue(std::string& value) {
  size_t pos = 0;
  while (pos < value.size() && value[pos] == ' ') {
    ++pos;
  }
  return value.substr(pos);
}


// explicit instantiations to separate header from implementation
template class cas::CsvImporter<cas::vint32_t>;
template class cas::CsvImporter<cas::vint64_t>;
template class cas::CsvImporter<cas::vstring_t>;
