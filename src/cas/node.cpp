#include "cas/node.hpp"
#include "cas/utils.hpp"
#include "cas/key_encoding.hpp"
#include "cas/node0.hpp"

#include <iostream>
#include <cctype>
#include <cassert>


cas::Node::Node(cas::NodeType type) : type_(type) {
}


cas::NodeType cas::Node::Type() {
  return type_;
}


bool cas::Node::IsPathNode() {
  return type_ == cas::NodeType::Path;
}


bool cas::Node::IsValueNode() {
  return type_ == cas::NodeType::Value;
}


void cas::Node::ForEachChild(uint8_t low, cas::ChildIt callback) {
  return ForEachChild(low, 0xFF, callback);
}


void cas::Node::ForEachChild(cas::ChildIt callback) {
  return ForEachChild(0x00, 0xFF, callback);
}


size_t cas::Node::PathPrefixSize() {
  return separator_pos_;
}


size_t cas::Node::ValuePrefixSize() {
  return prefix_.size() - separator_pos_;
}


void cas::Node::CollectStats(cas::IndexStats& stats, size_t depth) {
  stats.size_bytes_ += SizeBytes();
  ++stats.nr_nodes_;
  if (IsPathNode()) {
    ++stats.nr_path_nodes_;
  }
  if (IsValueNode()) {
    ++stats.nr_value_nodes_;
  }
  if (depth > stats.max_depth_) {
    stats.max_depth_ = depth;
  }
  switch (NodeWidth()) {
  case 0:
    ++stats.nr_node0_;
    break;
  case 4:
    if (IsPathNode()) {
      ++stats.nr_p_node4_;
    } else {
      assert(IsValueNode());
      ++stats.nr_v_node4_;
    }
    break;
  case 16:
    if (IsPathNode()) {
      ++stats.nr_p_node16_;
    } else {
      assert(IsValueNode());
      ++stats.nr_v_node16_;
    }
    break;
  case 48:
    if (IsPathNode()) {
      ++stats.nr_p_node48_;
    } else {
      assert(IsValueNode());
      ++stats.nr_v_node48_;
    }
    break;
  case 256:
    if (IsPathNode()) {
      ++stats.nr_p_node256_;
    } else {
      assert(IsValueNode());
      ++stats.nr_v_node256_;
    }
    break;
  default:
    assert(false);
  }

  auto it = stats.depth_histo_.find(depth);
  if (it == stats.depth_histo_.end()) {
    stats.depth_histo_[depth] = 0;
  }
  ++stats.depth_histo_[depth];

  ForEachChild([&](uint8_t, cas::Node& child) -> bool {
    if (IsPathNode() && child.IsPathNode()) {
      ++stats.pp_steps;
    } else if (IsPathNode() && child.IsValueNode()) {
      ++stats.pv_steps;
    } else if (IsValueNode() && child.IsPathNode()) {
      ++stats.vp_steps;
    } else if (IsValueNode() && child.IsValueNode()) {
      ++stats.vv_steps;
    }
    child.CollectStats(stats, depth + 1);
    return true;
  });
}


void cas::Node::Dump() {
  std::cout << "type_: ";
  switch (type_) {
  case cas::NodeType::Value:
    std::cout << "Value";
    break;
  case cas::NodeType::Path:
    std::cout << "Path";
    break;
  case cas::NodeType::Leaf:
    std::cout << "Leaf";
    break;
  }
  std::cout << std::endl;
  std::cout << "address: " << this << std::endl;
  std::cout << "nr_children_: " << (int)nr_children_ << std::endl;
  std::cout << "prefix_length_: " << prefix_.size() << std::endl;
  std::cout << "separator_pos_: " << separator_pos_ << std::endl;
  std::cout << "prefix_: ";
  cas::Utils::DumpHexValues(prefix_);
  std::cout << std::endl;
}


void cas::Node::DumpRecursive() {
  Dump();
  ForEachChild([](uint8_t, cas::Node& child) -> bool {
    child.DumpRecursive();
    return true;
  });
}


void cas::Node::DumpConcise(uint8_t edge_label, int indent) {
  std::string istring = std::string(2*indent, ' ');
  std::cout << istring;
  switch (type_) {
  case cas::NodeType::Path:
    std::cout << "[P] ";
    break;
  case cas::NodeType::Value:
    std::cout << "[V] ";
    break;
  case cas::NodeType::Leaf:
    std::cout << "[L] ";
    break;
  }
  if (std::isprint(edge_label)) {
    printf("0x%02X(%1c): ", (unsigned char) edge_label, edge_label);
  } else {
    printf("0x%02X():  ", (unsigned char) edge_label);
  }
  std::cout << "path[";
  /* cas::Utils::DumpChars(prefix_, separator_pos_); */
  cas::Utils::DumpHexValues(prefix_, separator_pos_);
  std::cout << "] ";
  std::cout << "value[";
  cas::Utils::DumpHexValues(prefix_, separator_pos_, prefix_.size());
  std::cout << "] ";
  // print capacity of prefixes
  std::cout << "cap(" << prefix_.capacity();
  if (IsLeaf()) {
    cas::Node0* self = static_cast<cas::Node0*>(this);
    std::cout << ";" << self->dids_.capacity() << "*" << sizeof(cas::did_t);
  }
  std::cout << ")";
  std::cout << std::endl;
  // recursively dump children (in ascending order)
  std::vector<std::pair<uint8_t, cas::Node*>> children;
  children.reserve(nr_children_);
  ForEachChild([&](uint8_t elabel, cas::Node& child) -> bool {
    children.push_back(std::make_pair(elabel, &child));
    return true;
  });
  for (auto i = children.rbegin(); i != children.rend(); ++i) {
    auto  elabel = std::get<0>(*i);
    auto* child  = std::get<1>(*i);
    child->DumpConcise(elabel, indent+1);
  }
}

void cas::Node::DumpConciseToFile(uint8_t edge_label, int indent, std::string file_name) {
  std::ofstream output_file(file_name, std::ios::app);
//  output_file.open(file_name);

  std::string istring = std::string(2*indent, ' ');
  output_file<< std::setfill( '0' );
  output_file << std::setw(2)<<istring;

  switch (type_) {
  case cas::NodeType::Path:
    output_file << "[P] ";
    break;
  case cas::NodeType::Value:
    output_file << "[V] ";
    break;
  case cas::NodeType::Leaf:
    output_file << "[L] ";
    break;
  }
  if (std::isprint(edge_label)) {
    output_file<<"0x"<< std::setw(2)<<std::uppercase << std::hex<<int(edge_label)<<"("<<edge_label<<"): ";
//      printf("0x%02X(%1c): ", (unsigned char) edge_label, edge_label);
  } else {
     output_file<<"0x"<< std::setw(2)<<std::uppercase<<std::hex<<int(edge_label)<<"(): ";
      //    printf("0x%02X():  ", (unsigned char) edge_label);
  }
  output_file << "path[";
  /* cas::Utils::DumpChars(prefix_, separator_pos_); */
  output_file.close();
  cas::Utils::DumpHexValuesToFile(prefix_, 0, separator_pos_, file_name);
  output_file = std::ofstream (file_name, std::ios::app);
  output_file << "] ";
  output_file << "value[";
  output_file.close();
  cas::Utils::DumpHexValuesToFile(prefix_, separator_pos_, prefix_.size(), file_name);
  output_file = std::ofstream (file_name, std::ios::app);
  output_file << "] ";
  // print capacity of prefixes
  output_file << "cap(" << prefix_.capacity();
  if (IsLeaf()) {
    cas::Node0* self = static_cast<cas::Node0*>(this);
//    output_file << ";" << self->dids_.capacity() << "*" << sizeof(cas::did_t);
    //Replaced upper line with
    output_file << ";" << self->dids_.size() << "*" << sizeof(cas::did_t);

  }
  output_file << ")";
  output_file << std::endl;
  // recursively dump children (in ascending order)
  std::vector<std::pair<uint8_t, cas::Node*>> children;
  children.reserve(nr_children_);
  ForEachChild([&](uint8_t elabel, cas::Node& child) -> bool {
    children.push_back(std::make_pair(elabel, &child));
    return true;
  });
  for (auto i = children.rbegin(); i != children.rend(); ++i) {
    auto  elabel = std::get<0>(*i);
    auto* child  = std::get<1>(*i);
    child->DumpConciseToFile(elabel, indent+1, file_name);
  }

}




void cas::Node::DumpLatex(Node* parent, uint8_t edge_label, int indent,
                          bool string_value) {
  std::string istring = std::string(2*indent, ' ');
  std::cout << istring << "[";
  std::cout << "{(";
  std::cout << "\\pstr{";
  if (separator_pos_ == 0) {
    std::cout << "$\\epsilon$";
  } else {
    cas::Utils::DumpChars(prefix_, 0, separator_pos_);
  }
  std::cout << "},\\vstr{";
  if (separator_pos_ == prefix_.size()) {
    std::cout << "$\\epsilon$";
  } else {
    if (string_value) {
      cas::Utils::DumpChars(prefix_, separator_pos_, prefix_.size());
    } else {
      for (size_t i = separator_pos_; i < prefix_.size(); ++i) {
        printf("%02X", prefix_[i]);
        if (i < prefix_.size()-1) {
          std::cout << "\\,";
        }
      }
    }
  }
  std::cout << "})";
  if (IsLeaf()) {
    std::cout << "\\\\$\\{";
    Node0* self = static_cast<Node0*>(this);
    for (size_t pos = 0; pos < self->dids_.size(); ++pos) {
      std::cout << "n_{" << self->dids_[pos] << "}";
      if (pos < self->dids_.size()-1) {
        std::cout << ",";
      }
    }
    std::cout << "\\}$";
  }
  std::cout << "},";
  if (parent == nullptr) {
    std::cout << "name=root";
  } else {
    std::cout << "elabel={east}{";
    switch (parent->type_) {
    case cas::NodeType::Path:
      if (edge_label == 0x00) {
        printf("\\pstr{\\$}");
      } else if (edge_label == 0xFF) {
        printf("\\pstr{/}");
      } else {
        printf("\\pstr{%c}", edge_label);
      }
      break;
    case cas::NodeType::Value:
      if (string_value) {
        printf("\\vstr{%c}", edge_label);
      } else {
        printf("\\vstr{%02X}", edge_label);
      }
      break;
    default:
      break;
    }
    std::cout << "},";
    switch (type_) {
    case cas::NodeType::Path:
      std::cout << "p";
      break;
    case cas::NodeType::Value:
      std::cout << "v";
      break;
    case cas::NodeType::Leaf:
      std::cout << "l";
      break;
    }
    printf("%c", parent->type_ == cas::NodeType::Path ? 'p' : 'v');
    std::cout << "node,";
  }
  std::cout << std::endl;
  // recursively dump children (in ascending order)
  std::vector<std::pair<uint8_t, cas::Node*>> children;
  children.reserve(nr_children_);
  ForEachChild([&](uint8_t elabel, cas::Node& child) -> bool {
    children.push_back(std::make_pair(elabel, &child));
    return true;
  });
  for (auto i = children.rbegin(); i != children.rend(); ++i) {
    auto  elabel = std::get<0>(*i);
    auto* child  = std::get<1>(*i);
    child->DumpLatex(this, elabel, indent+1, string_value);
  }
  // close this node
  std::cout << istring << "]" << std::endl;
}


void cas::Node::DumpBuffer(uint8_t *buffer, size_t length) {
  for (size_t i = 0; i < length; ++i) {
    printf("0x%02X", (unsigned char) buffer[i]);
    if (i < length-1) {
      printf(" ");
    }
  }
}


void cas::Node::DumpAddresses(cas::Node **buffer, size_t length) {
  for (size_t i = 0; i < length; ++i) {
    printf("%p", buffer[i]);
    if (i < length-1) {
      printf(" ");
    }
  }
}


std::vector<uint8_t> cas::Node::GetKeys() {
    return std::vector<uint8_t>();
}
