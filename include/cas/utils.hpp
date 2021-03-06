#ifndef CAS_UTILS_H_
#define CAS_UTILS_H_

#include <ostream>
#include <string>
#include <vector>
#include <cstdint>
#include <cstddef>
#include <iomanip>


namespace cas {

class Utils {
public:
  static void DumpHexValues(const std::vector<uint8_t>& buffer);

  static void DumpHexValues(const std::vector<uint8_t>& buffer, size_t size);

  static void DumpHexValues(const std::vector<uint8_t>& buffer, size_t offset, size_t size);

  static void DumpHexValuesToFile(const std::vector<uint8_t>& buffer, size_t offset, size_t size, std::string file_name);

  static void DumpChars(std::vector<uint8_t>& buffer);

  static void DumpChars(std::vector<uint8_t>& buffer, size_t size);

  static void DumpChars(std::vector<uint8_t>& buffer, size_t offset, size_t size);

  static void DumpChars(std::vector<uint8_t>& buffer, size_t offset,
                        size_t size, std::ostream& os);

  /*
   * returns:
   * -1 if lhs < rhs
   *  0 if lhs = rhs
   * +1 if lhs > rhs
   */
  static int Memcmp(const void* lhs, size_t len_lhs, const void* rhs, size_t len_rhs);

  static void DumpPath(const std::vector<std::string>& path, bool newline = true);

  static std::string JoinPath(const std::vector<std::string>& path);

  template<class VType>
  static std::string TypeToString();
};

} // namespace cas

#endif // CAS_UTILS_H_
