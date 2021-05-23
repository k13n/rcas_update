#ifndef CAS_INSERT_TYPE_H_
#define CAS_INSERT_TYPE_H_


namespace cas {

enum class MergeMethod {
  Slow,
  Fast
};

enum InsertType {
  StrictSlow,
  LazyFast
};

enum class InsertTarget {
  MainOnly,
  AuxiliaryOnly,
  MainAuxiliary,
};

struct InsertMethod {
  cas::InsertTarget target_;
  cas::InsertType main_insert_type_;
  cas::InsertType aux_insert_type_;
};

inline bool operator==(const InsertMethod& lhs, const InsertMethod& rhs) {
  return lhs.target_ == rhs.target_
    && lhs.main_insert_type_ == rhs.main_insert_type_
    && lhs.aux_insert_type_ == rhs.aux_insert_type_;
}


InsertMethod InsertMethodFromInt(int method);
int InsertMethodToInt(const InsertMethod& method);

MergeMethod MergeMethodFromInt(int method);
int MergeMethodToInt(const MergeMethod& method);

constexpr InsertMethod MainSS = {
  cas::InsertTarget::MainOnly,
  cas::InsertType::StrictSlow,
  cas::InsertType::StrictSlow
};
constexpr InsertMethod MainLF = {
  cas::InsertTarget::MainOnly,
  cas::InsertType::LazyFast,
  cas::InsertType::LazyFast
};
constexpr InsertMethod MainAuxLF = {
  cas::InsertTarget::MainAuxiliary,
  cas::InsertType::LazyFast,
  cas::InsertType::LazyFast
};
constexpr InsertMethod MainAuxSS = {
  cas::InsertTarget::MainAuxiliary,
  cas::InsertType::StrictSlow,
  cas::InsertType::StrictSlow
};



} // namespace cas

#endif //CAS_INSERT_TYPE_H_
