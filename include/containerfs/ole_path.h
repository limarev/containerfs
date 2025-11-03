#pragma once

#include <filesystem>

#include "ole_string.h"

#include <ostream>
#include <vector>

namespace ole {
class Path final {
public:
  Path() = default;

  operator std::filesystem::path() const { return {}; }
  static std::expected<Path, Error> make(const std::filesystem::path &path);

  [[nodiscard]] auto begin() const{ return paths_.begin(); }
  [[nodiscard]] auto end() const{ return paths_.end(); }
  auto empty() const noexcept { return paths_.empty(); }

  void append(const String& path) { paths_.push_back(path); }

  auto operator<=>(const Path &other) const noexcept = default;

private:
  Path(std::vector<String> paths);

private:
  std::vector<String> paths_;
};

inline std::ostream& operator<<(std::ostream &os, const Path &path) {
  for (const auto &p : path) {
    os << std::filesystem::path(static_cast<std::u16string_view>(p));
  }
  return os;
}

} // namespace ole
