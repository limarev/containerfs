#pragma once

#include "containerfs/device_api.h"

#include <iostream>
#include <algorithm>

inline static constexpr auto FREESECT   = 0xFFFFFFFF;
inline static constexpr auto ENDOFCHAIN = 0xFFFFFFFE;
inline static constexpr auto FATSECT    = 0xFFFFFFFD;
inline static constexpr auto DIFSECT    = 0xFFFFFFFC;

// ошибки драйвера OLE
enum class OleError : std::uint8_t {
  Success = 0,
  IoFailure = 1,
  InvalidSignature = 2,
  WrongByteOrder = 3,
  InvalidSectorShift = 4,
  InvalidMiniSectorShift = 5,
  InvalidMiniCutoff = 6,
  UnsupportedMajorVersion = 7,
  UnsupportedMinorVersion,
  InvalidCLSID,
  InvalidReservedField,
  InvalidNumberOfDirectorySectors
};

// POD заголовок (минимально нужные поля)
struct OleHeader {
  std::array<std::byte, 8> magic{};
  std::array<std::byte, 16> clsid{};
  std::uint16_t minor_version{};
  std::uint16_t major_version{};
  std::uint16_t byte_order{};
  std::uint16_t sector_shift{};
  std::uint16_t mini_sector_shift{};
  std::array<std::byte, 6> reserved{};
  std::uint32_t num_dir_sectors{};
  std::uint32_t num_fat_sectors{};
  std::uint32_t first_dir_sector{};
  std::uint32_t transaction_signature{};
  std::uint32_t mini_stream_cutoff_size{};
  std::uint32_t first_mini_fat_sector{};
  std::uint32_t num_mini_fat_sectors{};
  std::uint32_t first_difat_sector{};
  std::uint32_t num_difat_sectors{};
  std::array<std::uint32_t, 109> difat{};
};

template<typename Device>
std::expected<OleHeader, OleError> load_header(Device& device) {
    /** header всегда 512 байт:
     * For version 4 compound files, the header size (512 bytes) is less than the sector size (4,096 bytes),
     * so the remaining part of the header (3,584 bytes) MUST be filled with all zeroes.
     */
    std::array<std::byte, sizeof(OleHeader)> buffer{};
    if (!device.read_at(0, buffer)) {
      return std::unexpected(OleError::IoFailure);
    }

    OleHeader hdr{};
    const auto *p = buffer.data();

    auto read = [&]<typename T>(T &value) {
      if constexpr (std::is_array_v<T>) {
        std::copy_n(p, value.size(), reinterpret_cast<std::byte *>(value.data()));
        p += value.size();
      } else {
        std::copy_n(p, sizeof(T), reinterpret_cast<std::byte *>(&value));
        p += sizeof(T);
      }
    };

    read(hdr.magic);
    read(hdr.clsid);
    read(hdr.minor_version);
    read(hdr.major_version);
    read(hdr.byte_order);
    read(hdr.sector_shift);
    read(hdr.mini_sector_shift);
    read(hdr.reserved);
    read(hdr.num_dir_sectors);
    read(hdr.num_fat_sectors);
    read(hdr.first_dir_sector);
    read(hdr.transaction_signature);
    read(hdr.mini_stream_cutoff_size);
    read(hdr.first_mini_fat_sector);
    read(hdr.num_mini_fat_sectors);
    read(hdr.first_difat_sector);
    read(hdr.num_difat_sectors);
    for (auto &d : hdr.difat) {
      read(d);
    }

    /**
     * Header Signature (8 bytes): Identification signature for the compound file structure,
     * and MUST be set to the value 0xD0, 0xCF, 0x11, 0xE0, 0xA1, 0xB1, 0x1A, 0xE1.
     */
    if (constexpr std::array kMagic{std::byte{0xD0}, std::byte{0xCF}, std::byte{0x11}, std::byte{0xE0}, std::byte{0xA1},
                                    std::byte{0xB1}, std::byte{0x1A}, std::byte{0xE1}};
        hdr.magic != kMagic)
      return std::unexpected(OleError::InvalidSignature);

    /**
     * Header CLSID (16 bytes): Reserved and unused class ID that MUST be set to all zeroes (CLSID_NULL).
     */
    if (constexpr decltype(hdr.clsid) zeroes{}; hdr.clsid != zeroes)
      return std::unexpected(OleError::InvalidCLSID);

    /**
     * Major Version (2 bytes): Version number for breaking changes.
     * This field MUST be set to either 0x0003 (version 3) or 0x0004 (version 4).
     */
    if (hdr.major_version != 3 && hdr.major_version != 4)
      return std::unexpected(OleError::UnsupportedMajorVersion);

    /**
     * Minor Version (2 bytes): Version number for nonbreaking changes.
     * This field SHOULD be set to 0x003E if the major version field is either 0x0003 or 0x0004.
     */
    if (hdr.minor_version != 0x003E)
      return std::unexpected(OleError::UnsupportedMinorVersion);

    /**
     * Byte Order (2 bytes): This field MUST be set to 0xFFFE.
     * This field is a byte order mark for all integer fields, specifying little-endian byte order.
     */
    if (hdr.byte_order != 0xFFFE)
      return std::unexpected(OleError::WrongByteOrder);

    /**
    * Sector Shift (2 bytes): This field MUST be set to 0x0009, or 0x000c, depending on the Major Version field.
    * This field specifies the sector size of the compound file as a power of 2.
    * If Major Version is 3, the Sector Shift MUST be 0x0009, specifying a sector size of 512 bytes.
    * If Major Version is 4, the Sector Shift MUST be 0x000C, specifying a sector size of 4096 bytes.
     */
    if (const auto valid = (hdr.major_version == 3 && hdr.sector_shift == 9) || (hdr.major_version == 4 && hdr.sector_shift == 12);
      not valid)
      return std::unexpected(OleError::InvalidSectorShift);

    /**
     * Mini Sector Shift (2 bytes): This field MUST be set to 0x0006.
     * This field specifies the sector size of the Mini Stream as a power of 2.
     * The sector size of the Mini Stream MUST be 64 bytes.
     */
    if (hdr.mini_sector_shift != 6)
      return std::unexpected(OleError::InvalidMiniSectorShift);

    /**
     * Reserved (6 bytes): This field MUST be set to all zeroes.
     */
    if (constexpr decltype(hdr.reserved) zeroes{}; hdr.reserved != zeroes)
      return std::unexpected(OleError::InvalidReservedField);

    /**
     * Number of Directory Sectors (4 bytes):
     * This integer field contains the count of the number of directory sectors in the compound file.
     * If Major Version is 3, the Number of Directory Sectors MUST be zero.
     * This field is not supported for version 3 compound files.
     */
    if (constexpr decltype(hdr.num_dir_sectors) zeroes{}; hdr.major_version == 3 && hdr.num_dir_sectors != zeroes)
      return std::unexpected(OleError::InvalidNumberOfDirectorySectors);

    /**
     * Mini Stream Cutoff Size (4 bytes): This integer field MUST be set to 0x00001000.
     * This field specifies the maximum size of a user-defined data stream that is allocated from the mini FAT and mini stream,
     * and that cutoff is 4096 bytes. Any user-defined data stream that is greater than or equal to this cutoff size
     * must be allocated as normal sectors from the FAT.
     */
    if (hdr.mini_stream_cutoff_size != 4096) {
      return std::unexpected(OleError::InvalidMiniCutoff);
    }

  return hdr;
}

std::uint64_t sector_offset(uint32_t sector, uint32_t sector_size) noexcept {
  return static_cast<std::uint64_t>(sector + 1) * sector_size;
}

template<typename Device>
std::expected<std::vector<uint32_t>, OleError> load_fat(Device& device, const OleHeader& header) {
  const auto sector_size = 1 << header.sector_shift;
  size_t total_fat_entries = 0;
  for (auto sector : header.difat) {
    if (sector != FREESECT) total_fat_entries += sector_size / sizeof(uint32_t);
  }

  std::vector<uint32_t> fat;
  fat.resize(total_fat_entries);

  size_t offset = 0;
  for (auto sector : header.difat) {
    if (sector == FREESECT) continue;

    std::vector<std::byte> buf(sector_size);
    if (!device.read_at(sector_offset(sector, sector_size), buf))
      return std::unexpected(OleError::IoFailure);
    // каждая запись FAT = uint32_t
    auto* p = reinterpret_cast<uint32_t*>(buf.data());
    size_t n = sector_size / sizeof(uint32_t);
    std::copy_n(p, n, fat.begin() + offset);
    offset += n;
  }

  // TODO: загрузить цепочку дополнительных DIFAT-секторов, если hdr_.num_difat_sectors > 0
  return fat;
}

template <typename Device> class OleDriver final {
public:
  using error_type = OleError;

  static std::expected<OleDriver, error_type> create(Device &&dev) {
    auto header = load_header(dev);
    if (not header) {
      return std::unexpected(header.error());
    }

    auto fat = load_fat(dev, *header);
    if (not fat) {
      return std::unexpected(fat.error());
    }

    OleDriver driver{std::move(dev)};
    driver.hdr_ = *header;

    return driver;
  }

  // --- публичный API (пока заглушки) ---
  std::vector<std::byte> read_file(const std::filesystem::path &) { return {}; }
  bool exists(const std::filesystem::path &) const noexcept { return false; }
  int file_size(const std::filesystem::path &) const noexcept { return -1; }
  bool is_directory(const std::filesystem::path &) const noexcept { return false; }

private:
  explicit OleDriver(Device &&dev) : dev_{std::move(dev)} {}

  Device dev_;
  OleHeader hdr_{};
};
