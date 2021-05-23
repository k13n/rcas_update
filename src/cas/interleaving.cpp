#include "cas/interleaving.hpp"
#include "cas/key_encoding.hpp"
#include "cas/interleaved_key.hpp"
#include "cas/interleaver.hpp"
#include "cas/node.hpp"
#include "cas/utils.hpp"

#include <iostream>
#include <cassert>
#include <cmath>


template<class VType>
cas::Interleaving<VType>::Interleaving(cas::Cas<VType>& index,
    cas::BinaryKey& key)
  : index_(index)
  , key_(key)
  , node_(index.root_)
{
}


template<class VType>
void cas::Interleaving<VType>::Compute() {
  bool success = Descend();
  if (success) {
    /* DumpInterleaving(); */
    DumpInterleavingLatex();
  }
}


template<class VType>
bool cas::Interleaving<VType>::Descend() {
  uint8_t disc_byte = 0x00;

  while (true) {
    Tuple tuple;
    size_t pos = 0;
    bool success = false;

    if (node_ != index_.root_) {
      switch (result_.back().dimension_) {
      case cas::NodeType::Path:
        tuple.path_bytes_.push_back(disc_byte);
        break;
      case cas::NodeType::Value:
        tuple.value_bytes_.push_back(disc_byte);
        break;
      default:
        assert(false);
      }
    }

    // Deal with path bytes
    success = MatchPath(pos);
    if (!success) {
      std::cout << "Path not matched" << std::endl;
      return false;
    }
    std::copy(node_->prefix_.begin(), node_->prefix_.begin() + pos,
        std::back_inserter(tuple.path_bytes_));

    // Deal with value bytes
    pos = node_->separator_pos_;
    success = MatchValue(pos);
    if (!success) {
      std::cout << "Value not matched" << std::endl;
      return false;
    }
    std::copy(node_->prefix_.begin() + node_->separator_pos_,
        node_->prefix_.begin() + pos, std::back_inserter(tuple.value_bytes_));

    // Set dimension
    tuple.dimension_ = node_->type_;
    result_.push_back(tuple);

    switch (node_->type_) {
    case cas::NodeType::Leaf:
      return p_pos_ == key_.path_.size()
        && v_pos_ == key_.value_.size();
    case cas::NodeType::Path:
      disc_byte = key_.path_[p_pos_];
      ++p_pos_;
      break;
    case cas::NodeType::Value:
      disc_byte = key_.value_[v_pos_];
      ++v_pos_;
      break;
    }

    node_ = node_->LocateChild(disc_byte);
    if (node_ == nullptr) {
      std::cout << "Child not found for byte: ";
      printf("0x%02X\n", (unsigned char) disc_byte);
      return false;
    }
  }
}


template<class VType>
bool cas::Interleaving<VType>::MatchPath(size_t& pos) {
  while (p_pos_ < key_.path_.size()
      && pos < node_->separator_pos_
      && key_.path_[p_pos_] == node_->prefix_[pos]) {
    ++p_pos_;
    ++pos;
  }
  return pos == node_->separator_pos_;
}


template<class VType>
bool cas::Interleaving<VType>::MatchValue(size_t& pos) {
  while (v_pos_ < key_.value_.size()
      && pos < node_->prefix_.size()
      && key_.value_[v_pos_] == node_->prefix_[pos]) {
    ++v_pos_;
    ++pos;
  }
  return pos == node_->prefix_.size();
}


template<class VType>
void cas::Interleaving<VType>::DumpInterleaving() {
  auto DumpPath = [](std::vector<uint8_t> buffer) -> void {
    std::cout << "'";
    std::cout << COL_BLUE;
    cas::Utils::DumpChars(buffer);
    std::cout << COL_NONE;
    std::cout << "'";
  };
  auto DumpValue = [](std::vector<uint8_t> buffer) -> void {
    std::cout << "'";
    std::cout << COL_RED;
    for (size_t i = 0; i < buffer.size(); ++i) {
      printf("%02X", (unsigned char) buffer[i]);
      if (i < buffer.size()-1) {
        printf(" ");
      }
    }
    std::cout << COL_NONE;
    std::cout << "'";
  };

  cas::NodeType dimension = cas::NodeType::Value;
  for (const auto& tuple : result_) {
    std::cout << "(";
    switch (dimension) {
    case cas::NodeType::Path:
      DumpPath(tuple.path_bytes_);
      std::cout << ", ";
      DumpValue(tuple.value_bytes_);
      break;
    case cas::NodeType::Value:
      DumpValue(tuple.value_bytes_);
      std::cout << ", ";
      DumpPath(tuple.path_bytes_);
      break;
    default:
      std::cout << "dimension cannot be Leaf" << std::endl;
      assert(false);
    }
    std::cout << ", ";
    dimension = tuple.dimension_;
    switch (dimension) {
    case cas::NodeType::Path:
      std::cout << "P"; break;
    case cas::NodeType::Value:
      std::cout << "V"; break;
    case cas::NodeType::Leaf:
      std::cout << "L"; break;
    }

    std::cout << ")";
    if (&tuple != &result_.back()) {
      std::cout << ", ";
    }
  }
  std::cout << std::endl;
}


template<class VType>
void cas::Interleaving<VType>::DumpInterleavingLatex() {
  auto DumpPath = [](std::vector<uint8_t> buffer, bool dsc) -> void {
    std::cout << "\\ptstr{";
    if (buffer.size() == 0) {
      std::cout << "$\\epsilon$";
    } else {
      /* cas::Utils::DumpChars(buffer); */
      for (size_t i = 0; i < buffer.size(); ++i) {
        std::string symbol;
        if (buffer[i] == cas::kPathSep) {
          symbol = "/";
        } else if (buffer[i] == cas::kNullByte) {
          symbol = "\\$";
        } else if (buffer[i] == '_') {
          symbol = "\\_";
        } else {
          symbol = static_cast<unsigned char>(buffer[i]);
        }
        if (dsc && i == 0) {
          std::cout << "\\ptbstr{";
        }
        /* std::cout << COL_BLUE; */
        std::cout << symbol;
        /* std::cout << COL_NONE; */
        if (dsc && i == 0) {
          std::cout << "}";
        }
      }
    }
    std::cout << "}";
  };
  auto DumpValue = [](std::vector<uint8_t> buffer, bool dsc) -> void {
    std::cout << "\\vtstr{";
    if (buffer.size() == 0) {
      std::cout << "$\\epsilon$";
    } else {
      for (size_t i = 0; i < buffer.size(); ++i) {
        if (dsc && i == 0) {
          std::cout << "\\vtbstr{";
        }
        /* std::cout << COL_RED; */
        printf("%02X", (unsigned char) buffer[i]);
        /* std::cout << COL_NONE; */
        if (dsc && i == 0) {
          std::cout << "}";
        }
        if (i < buffer.size()-1) {
          printf("\\,");
        }
      }
    }
    std::cout << "}";
  };

  cas::NodeType dimension = cas::NodeType::Value;
  std::cout << "(";
  for (const auto& tuple : result_) {
    std::cout << "$(";
    switch (dimension) {
    case cas::NodeType::Path:
      DumpPath(tuple.path_bytes_, true);
      std::cout << ", ";
      DumpValue(tuple.value_bytes_, false);
      break;
    case cas::NodeType::Value:
      DumpValue(tuple.value_bytes_, true);
      std::cout << ", ";
      DumpPath(tuple.path_bytes_, false);
      break;
    default:
      std::cout << "dimension cannot be Leaf" << std::endl;
      assert(false);
    }
    std::cout << ", ";
    dimension = tuple.dimension_;
    switch (dimension) {
    case cas::NodeType::Path:
      std::cout << "P"; break;
    case cas::NodeType::Value:
      std::cout << "V"; break;
    case cas::NodeType::Leaf:
      std::cout << "\\bot"; break;
    }

    std::cout << ")$";
    if (&tuple != &result_.back()) {
      std::cout << ",";
    }
    /* std::cout << "%" << std::endl; */
  }
  std::cout << ")" << std::endl;
}


// explicit instantiations to separate header from implementation
template class cas::Interleaving<cas::vint32_t>;
template class cas::Interleaving<cas::vint64_t>;
template class cas::Interleaving<cas::vstring_t>;
