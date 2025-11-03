#pragma once

#include <cstdint>

namespace ole {
enum class Error : std::uint8_t {
  Success = 0,
  IoFailure = 1,
  InvalidSignature = 2,
  WrongByteOrder = 3,
  InvalidSectorShift = 4,
  InvalidMiniSectorShift = 5,
  InvalidMiniCutoff = 6,
  UnsupportedMajorVersion = 7,
  UnsupportedMinorVersion = 8,
  InvalidCLSID = 9,
  InvalidReservedField = 10,
  InvalidNumberOfDirectorySectors = 11,
  CorruptedFile = 12,
  MiniFatHeaderInconsistent = 13,
  Exceeds32UTF16CodePoints = 14,
  ContainsIllegalCharacters = 15,
  Exceeds62Bytes = 16,
  Exceeds64Bytes = 17,
  NotMultipleOf2 = 18,
  NotNullTerminated = 19
};
} // namespace ole