#pragma once

#include "device_api.h"
#include "file_device.h"
#include "namespace.h"

#include <vector>

CONTAINERFS_NAMESPACE_BEGIN

template <FileSystemDriver Driver>
class FileSystem final {
public:
  using error_type = Driver::error_type;

  // For convenient using
  // Для сохранения семантики mount<OleDriver>(Device {...});
  // Так видно, что монтируется файловая система и используется такой-то драйвер
  // Простой конструктор не подходит, если мы хотим использовать std::expected в качестве основного механизма возврата ошибок
  template<template <ReadableDevice> class DriverT, ReadableDevice Device>
  friend auto mount(Device&& dev) -> std::expected<FileSystem<DriverT<Device>>, typename DriverT<Device>::error_type>;

  std::vector<std::byte> read_file(std::filesystem::path const& path) {
    return driver_.read_file(path);
  }
  bool exists(std::filesystem::path const& path) const noexcept { return driver_.exists(path); }
  int file_size(std::filesystem::path const& path) const noexcept { return driver_.file_size(path); }
  bool is_directory(std::filesystem::path const& path) const noexcept { return driver_.is_directory(path); }

private:
  explicit FileSystem(Driver&& driver): driver_{std::move(driver)} {}
  Driver driver_;
};

template<template <ReadableDevice> class Driver, ReadableDevice Device>
auto mount(Device&& dev) -> std::expected<FileSystem<Driver<Device>>, typename Driver<Device>::error_type> {
  using D = Driver<Device>;
  auto expected = D::create(std::forward<Device>(dev));
  if (!expected) {
    return std::unexpected(expected.error());
  }
  return FileSystem<D>(std::move(*expected));
}

CONTAINERFS_NAMESPACE_END