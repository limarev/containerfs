#pragma once

#include <array>
#include <expected>
#include <filesystem>

#include "ole_error.h"

namespace ole {
class String final {
public:
  static constexpr std::size_t kBytes = 64;
  static constexpr std::size_t kUnits = kBytes / 2; // 32 code units включая '\0'
  static constexpr auto kIllegal = {u'/', u'\\', u':', u'!'};

  String() = default;
  static std::expected<String, Error> make(std::u16string_view src);
  static std::expected<String, Error> make(std::filesystem::path src);
  static std::expected<String, Error> make(std::array<std::byte, kBytes> raw, std::size_t size);

  [[nodiscard]] std::uint16_t size_bytes() const noexcept;
  [[nodiscard]] std::strong_ordering compare(const String &other) const noexcept;
  friend std::strong_ordering operator<=>(String const& a, String const& b) noexcept;
  bool operator==(const String & other) const noexcept { return this->compare(other) == 0; }
  operator std::u16string_view() const noexcept { return s_; }

private:
  explicit String(std::u16string_view src) : s_(src) {}
  explicit String(std::u16string src) : s_(std::move(src)) {}
  std::u16string s_;
};

std::strong_ordering operator<=>(String const& a, String const& b) noexcept;
std::strong_ordering compare(String const& a, String const& b) noexcept;
} // namespace ole