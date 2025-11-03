#include "string.h"

#include <algorithm>
#include <cassert>
#include <optional>
#include <span>

template<typename T> concept u16contiguous_range = std::ranges::contiguous_range<T> && sizeof(std::ranges::range_value_t<T>) == 2;

std::optional<ole::Error> is_invalid(u16contiguous_range auto src) {
  const auto size_in_bytes = src.size()*sizeof(std::u16string_view::value_type);
  /**
   * The name MUST be terminated with a UTF-16 terminating null character.
   */
  if (size_in_bytes > ole::String::kBytes - 2)
    return ole::Error::Exceeds62Bytes;

  // TODO storage and stream names are limited to 32 UTF-16 code points, including the terminating null character
  // Now we are using only ASCII symbols

  using std::ranges::any_of;
  constexpr auto is_illegal = [](auto ch) { return any_of(ole::String::kIllegal, [&ch](auto c) { return c == ch; }); };
  if (any_of(src, is_illegal))
    return ole::Error::ContainsIllegalCharacters;

  return std::nullopt;
}

std::expected<ole::String, ole::Error> ole::String::make(std::u16string_view src) {
  if (auto error = is_invalid(src); error.has_value())
    return std::unexpected(*error);

  return String(src);
}

std::expected<ole::String, ole::Error> ole::String::make(std::filesystem::path src) {
  if (auto error = is_invalid(src.u16string()); error.has_value())
    return std::unexpected(*error);

  return ole::String(src.u16string());
}

std::expected<ole::String, ole::Error> ole::String::make(std::array<std::byte, kBytes> raw, std::size_t size_in_bytes) {
  assert(size_in_bytes != 0);

  if (size_in_bytes > kBytes) {
    return std::unexpected{Error::Exceeds64Bytes};
  }

  if (size_in_bytes % 2 != 0) {
    return std::unexpected{Error::NotMultipleOf2};
  }

  std::u16string result(raw.size() / sizeof(char16_t), u'\0');
  auto view = as_writable_bytes(std::span(result));
  std::ranges::move(raw, view.begin());
  result.resize(size_in_bytes / 2);

  if (result.back() != u'\0') {
    return std::unexpected{Error::NotNullTerminated};
  }

  result.pop_back(); // мы не храним u'\0'

  if (auto error = is_invalid(result); error.has_value())
    return std::unexpected(*error);

  return String(std::move(result));
}

std::strong_ordering ole::operator<=>(String const& a, String const& b) noexcept { return a.compare(b); }

std::strong_ordering ole::String::compare(const String &other) const noexcept {
  using std::strong_ordering;

  // 1) Короче имя — меньше
  if (s_.size() != other.s_.size()) {
    return (s_.size() < other.s_.size()) ? strong_ordering::less
                                         : strong_ordering::greater;
  }

  // 2) Равная длина: поэлементно сравниваем code units после uppercasing
  constexpr auto is_surrogate = [](char16_t ch) noexcept {
    return ch >= 0xD800 && ch <= 0xDFFF;
  };

  // Simple case conversion (ASCII; не трогаем суррогаты и не-ASCII)
  auto to_upper_simple = [&](char16_t ch) noexcept -> char16_t {
    if (is_surrogate(ch)) return ch;               // суррогаты не uppercased
    if (ch >= u'a' && ch <= u'z') return ch - (u'a' - u'A'); // ASCII
    return ch;
  };

  for (std::size_t i = 0; i < s_.size(); ++i) {
    const char16_t a = to_upper_simple(s_[i]);
    const char16_t b = to_upper_simple(other.s_[i]);
    if (a != b) {
      return (a < b) ? strong_ordering::less : strong_ordering::greater;
    }
  }

  return strong_ordering::equal;
}

std::uint16_t ole::String::size_bytes() const noexcept {
  // длина в байтах включая завершающий ноль
  return static_cast<std::uint16_t>(s_.size() * sizeof(char16_t));
}

std::strong_ordering ole::compare(String const& a, String const& b) noexcept {
  return a.compare(b);
}