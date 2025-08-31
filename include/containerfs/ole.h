#pragma once

#include "containerfs/device_api.h"

enum class OleError: std::uint8_t {
  Success = 0,
  FileNotFound = 1,
};

template<typename Device>
class OleDriver final {
public:
  using error_type = OleError;
  static std::expected<OleDriver, error_type> create(Device&& dev) {
    return std::unexpected(error_type::FileNotFound);
  }

  std::vector<std::byte> read_file(std::filesystem::path const& path) {
    return{};
  }
  bool exists(std::filesystem::path const& path) const noexcept { return false; }
  int file_size(std::filesystem::path const& path) const noexcept { return -1; }
  bool is_directory(std::filesystem::path const& path) const noexcept { return false; }

private:
  explicit OleDriver(Device&& dev): dev_{std::move(dev)} {}
  Device dev_;
};