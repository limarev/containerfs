#pragma once

#include "namespace.h"

#include "error.h"
#include <bit>
#include <expected>
#include <filesystem>
#include <span>
#include <vector>

CONTAINERFS_NAMESPACE_BEGIN

template<class Dev>
concept ReadableDevice = requires(Dev& d, std::uint64_t off, std::span<std::byte> dst) {
  { d.read_at(off, dst) } -> std::same_as<bool>;
  // { d.size() } -> std::same_as<std::uint64_t>;
};

template<class Dev>
concept WritableDevice = ReadableDevice<Dev> && requires(Dev& d, std::uint64_t off, std::span<const std::byte> src) {
  { d.write_at(off, src) } -> std::same_as<bool>;
};

// Концепт, который умеет «монтировать» контейнер из устройства
template<class Dev, class FS>
concept MountableDevice = ReadableDevice<Dev> && requires(Dev& dev) {
  { mount(dev) } -> std::same_as<std::expected<FS, Error>>;
};

template<class D>
concept FileSystemDriver = requires(D d, std::filesystem::path p) {
  { d.read_file(p) } -> std::same_as<std::vector<std::byte>>;
  { d.exists(p) }    -> std::same_as<bool>;
  { d.file_size(p) } -> std::same_as<int>;
  { d.is_directory(p) } -> std::same_as<bool>;
};

CONTAINERFS_NAMESPACE_END