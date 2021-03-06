#include "cas/path_matcher.hpp"
#include "cas/key_encoding.hpp"
#include "cas/search_key.hpp"
#include <iostream>
#include <cassert>


cas::PathMatcher::PrefixMatch cas::PathMatcher::MatchPathIncremental(
    const std::vector<uint8_t>& path,
    const cas::BinaryQP& qpath,
    size_t len_path,
    State& s) {
  const auto& query_path = qpath.bytes_;
  while (s.ppos_ < len_path &&
         s.desc_ppos_ < len_path &&
         path[s.ppos_] != '\0') {
    if (s.qpos_ < query_path.size() && path[s.ppos_] == query_path[s.qpos_]) {
      // simple pattern symbol matches input symbol
      ++s.ppos_;
      ++s.qpos_;
    }
    else if (s.qpos_ < query_path.size() &&
               query_path[s.qpos_] == cas::kByteChild) {
      // child axis is fully matched when the next path
      // separator occurs
      if (path[s.ppos_] == cas::kPathSep) {
        ++s.qpos_;
      } else {
        ++s.ppos_;
      }
    }
    else if (s.qpos_ < query_path.size() &&
               query_path[s.qpos_] == cas::kByteDescendantOrSelf &&
               path[s.ppos_] == cas::kPathSep) {
      // remember were we found the last descendant-or-self axis
      s.desc_ppos_ = s.ppos_ + 1;
      s.desc_qpos_ = s.qpos_;
      ++s.qpos_;
      ++s.ppos_;
    } else if (s.desc_qpos_ != -1) {
      // the descendant-or-self axis must match a larger substring
      if (path[s.desc_ppos_] == cas::kPathSep) {
        s.ppos_ = s.desc_ppos_ + 1;
        s.qpos_ = s.desc_qpos_ + 1;
        ++s.desc_ppos_;
      } else if (path[s.desc_ppos_] == '\0') {
        // matched the descendant-or-self axis until the end
        s.ppos_ = s.desc_ppos_;
        s.qpos_ = s.desc_qpos_ + 1;
      } else {
        ++s.desc_ppos_;
      }
    } else {
      return MISMATCH;
    }
  }

  // we need more input characters to determine the outcome
  if (s.ppos_ >= len_path || s.desc_ppos_ >= len_path) {
    return INCOMPLETE;
  }

  // we reached the end of a full path. now we can determine
  // if we fully matched it or not
  assert(path[s.ppos_] == '\0');
  if (s.qpos_ < query_path.size() &&
      query_path[s.qpos_] == cas::kByteChild &&
      path[s.ppos_-1] != cas::kPathSep) {
    // an empty suffix matches the pattern '?' only if the
    // last input character was an actual character
    ++s.qpos_;
  }
  while (s.qpos_ < query_path.size() &&
         query_path[s.qpos_] == cas::kByteDescendantOrSelf) {
    // an arbitrary number of descendant-or-self steps
    // match the empty suffix
    ++s.qpos_;
  }

  return (s.qpos_ == query_path.size()) ? MATCH : MISMATCH;
}


bool cas::PathMatcher::MatchPath(
    const std::vector<uint8_t>& path,
    const cas::BinaryQP& query_path) {
  State s;
  return MatchPathIncremental(path, query_path, path.size(), s) == MATCH;
}


void cas::PathMatcher::State::Dump() {
  std::cout << "ppos_: " << ppos_ << std::endl;
  std::cout << "qpos_: " << qpos_ << std::endl;
  std::cout << "desc_ppos_: " << desc_ppos_ << std::endl;
  std::cout << "desc_qpos_: " << desc_qpos_ << std::endl;
}

