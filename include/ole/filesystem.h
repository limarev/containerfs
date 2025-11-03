#pragma once

#include "error.h"
#include "file_device.h"
#include "directory.h"
#include <cstddef>

namespace ole {
class Path;

class Filesystem final {
public:
  using error_type = Error;

  static std::expected<Filesystem, error_type> mount(FileDevice &&dev);

  // --- публичный API (пока заглушки) ---
  std::vector<std::byte> read_file(const std::filesystem::path &) { return {}; }

  [[nodiscard]] bool exists(const Path& path) const noexcept;

  [[nodiscard]] std::expected<int, error_type> file_size(const Path& path) const noexcept;
  [[nodiscard]] bool is_directory(const Path& path) const noexcept;
  [[nodiscard]] bool is_regular_file(const Path& path) const noexcept;

private:
  explicit Filesystem(FileDevice &&dev);

  FileDevice dev_;
  std::vector<DirectoryEntry> dirs_ {};
};
}