#pragma once

#include <filesystem>
#include <span>
#include <vector>

namespace ole {
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
// template<class Dev, class FS>
// concept MountableDevice = ReadableDevice<Dev> && requires(Dev& dev) {
//   { mount(dev) } -> std::same_as<std::expected<FS, Error>>;
// };

template<typename T>
concept PathConvertible = std::convertible_to<T, std::filesystem::path>;
}