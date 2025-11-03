#pragma once

#include <filesystem>
#include <fstream>
#include <span>

namespace ole {
class FileDevice final {
public:
  explicit FileDevice(const std::filesystem::path& path): dev_{std::make_unique<std::ifstream>(path, std::ios::binary | std::ios::in)} {}

  bool read_at(std::uint64_t off, std::span<std::byte> dst) {
    if (not dev_->seekg(static_cast<int64_t>(off), std::ios::beg)) {
      return false;
    }

    if (not dev_->read(reinterpret_cast<char*>(dst.data()), static_cast<std::streamsize>(dst.size()))) {
      return false;
    }

    return true;
  }
private:
  std::unique_ptr<std::istream> dev_;
};
}