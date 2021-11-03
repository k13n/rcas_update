#include "cas/update_type.hpp"
#include <stdexcept>

cas::InsertMethod cas::InsertMethodFromInt(int method) {
  switch (method) {
    case 0: return cas::MainSS;
    case 1: return cas::MainLF;
    case 2: return cas::MainAuxLF;
    case 3: return cas::MainAuxSS;
    default:
      throw std::runtime_error{"unknown method!"};
  }
}

int cas::InsertMethodToInt(const cas::InsertMethod& method) {
  if (method == cas::MainSS)         { return 0; }
  else if (method == cas::MainLF)    { return 1; }
  else if (method == cas::MainAuxLF) { return 2; }
  else if (method == cas::MainAuxSS) { return 3; }
  throw std::runtime_error{"unknown method!"};
}


cas::MergeMethod cas::MergeMethodFromInt(int method) {
  switch (method) {
    case 0: return cas::MergeMethod::Slow;
    case 1: return cas::MergeMethod::Fast;
    default:
      throw std::runtime_error{"unknown method!"};
  }
}

int cas::MergeMethodToInt(const cas::MergeMethod& method) {
  switch (method) {
    case cas::MergeMethod::Slow: return 0;
    case cas::MergeMethod::Fast: return 1;
    default:
      throw std::runtime_error{"unknown method!"};
  }
}
