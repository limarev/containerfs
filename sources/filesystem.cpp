#include "ole/filesystem.h"
#include <cassert>

namespace ole {

constexpr std::uint32_t FREESECT = 0xFFFFFFFF;
constexpr std::uint32_t ENDOFCHAIN = 0xFFFFFFFE;
constexpr std::uint32_t FATSECT = 0xFFFFFFFD;
constexpr std::uint32_t DIFSECT = 0xFFFFFFFC;

using fat_t = std::uint32_t;

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

struct DirectoryEntryRaw {
  std::byte object_type;
  std::byte color_flag;
  std::uint16_t name_size_in_bytes;
  std::array<std::byte, 16> clsid;
  std::uint32_t left_id;
  std::uint32_t right_id;
  std::uint32_t child_id;
  std::uint32_t state_bits;
  std::uint32_t starting_sector;
  std::uint64_t creation_time;
  std::uint64_t modified_time;
  std::uint64_t stream_size; // in bytes
  std::array<std::byte, 64> name;

  auto operator<=>(const DirectoryEntryRaw &) const = default;
};
}

constexpr auto operator|(std::ranges::range auto &&src, std::ranges::sized_range auto &dst) {
  auto d = std::as_writable_bytes(std::span(dst));
  auto res = std::ranges::copy_n(src.begin(), d.size(), d.begin());
  return std::ranges::subrange(res.in, src.end());
}

constexpr auto operator|(std::ranges::range auto &&src, std::integral auto &dst) {
  std::span<std::byte, sizeof(dst)> d{reinterpret_cast<std::byte *>(std::addressof(dst)), sizeof(dst)};
  return src | d;
}

constexpr auto operator|(std::ranges::range auto &&src, std::byte &dst) {
  auto res = std::ranges::copy_n(src.begin(), sizeof(dst), std::addressof(dst));
  return std::ranges::subrange(res.in, src.end());
}

// Смещение сектора (SID) в байтах (OLE: заголовок = "нулевой сектор")
constexpr auto sector_offset (ole::fat_t sid, std::uint16_t sector_size) -> std::uint64_t {
  return (sid + 1) * static_cast<std::uint64_t>(sector_size);
}

template <bool Validate = true>
std::expected<ole::OleHeader, ole::Error> load_header(ole::FileDevice &device) {
  /** header всегда 512 байт:
   * For version 4 compound files, the header size (512 bytes) is less than the sector size (4,096 bytes),
   * so the remaining part of the header (3,584 bytes) MUST be filled with all zeroes.
   */
  static_assert(sizeof(ole::OleHeader) == 512);
  std::array<std::byte, sizeof(ole::OleHeader)> buffer{};
  if (!device.read_at(0, buffer)) {
    return std::unexpected(ole::Error::IoFailure);
  }
  // TODO bitcast to wire type
  ole::OleHeader hdr{};
  [[ maybe_unused ]] const auto tail = buffer
  | hdr.magic
  | hdr.clsid
  | hdr.minor_version
  | hdr.major_version
  | hdr.byte_order
  | hdr.sector_shift
  | hdr.mini_sector_shift
  | hdr.reserved
  | hdr.num_dir_sectors
  | hdr.num_fat_sectors
  | hdr.first_dir_sector
  | hdr.transaction_signature
  | hdr.mini_stream_cutoff_size
  | hdr.first_mini_fat_sector
  | hdr.num_mini_fat_sectors
  | hdr.first_difat_sector
  | hdr.num_difat_sectors
  | hdr.difat;

  assert(empty(tail));

  if constexpr (Validate) {
    /**
     * Header Signature (8 bytes): Identification signature for the compound file structure,
     * and MUST be set to the value 0xD0, 0xCF, 0x11, 0xE0, 0xA1, 0xB1, 0x1A, 0xE1.
     */
    if (constexpr std::array kMagic{std::byte{0xD0}, std::byte{0xCF}, std::byte{0x11}, std::byte{0xE0}, std::byte{0xA1},
                                    std::byte{0xB1}, std::byte{0x1A}, std::byte{0xE1}};
        hdr.magic != kMagic)
      return std::unexpected(ole::Error::InvalidSignature);

    /**
     * Header CLSID (16 bytes): Reserved and unused class ID that MUST be set to all zeroes (CLSID_NULL).
     */
    if (constexpr decltype(hdr.clsid) zeroes{}; hdr.clsid != zeroes)
      return std::unexpected(ole::Error::InvalidCLSID);

    /**
     * Major Version (2 bytes): Version number for breaking changes.
     * This field MUST be set to either 0x0003 (version 3) or 0x0004 (version 4).
     */
    if (hdr.major_version != 3 && hdr.major_version != 4)
      return std::unexpected(ole::Error::UnsupportedMajorVersion);

    /**
     * Minor Version (2 bytes): Version number for nonbreaking changes.
     * This field SHOULD be set to 0x003E if the major version field is either 0x0003 or 0x0004.
     */
    if (hdr.minor_version != 0x003E)
      return std::unexpected(ole::Error::UnsupportedMinorVersion);

    /**
     * Byte Order (2 bytes): This field MUST be set to 0xFFFE.
     * This field is a byte order mark for all integer fields, specifying little-endian byte order.
     */
    if (hdr.byte_order != 0xFFFE)
      return std::unexpected(ole::Error::WrongByteOrder);

    /**
     * Sector Shift (2 bytes): This field MUST be set to 0x0009, or 0x000c, depending on the Major Version field.
     * This field specifies the sector size of the compound file as a power of 2.
     * If Major Version is 3, the Sector Shift MUST be 0x0009, specifying a sector size of 512 bytes.
     * If Major Version is 4, the Sector Shift MUST be 0x000C, specifying a sector size of 4096 bytes.
     */
    if (const auto valid =
            (hdr.major_version == 3 && hdr.sector_shift == 9) || (hdr.major_version == 4 && hdr.sector_shift == 12);
        not valid)
      return std::unexpected(ole::Error::InvalidSectorShift);

    /**
     * Mini Sector Shift (2 bytes): This field MUST be set to 0x0006.
     * This field specifies the sector size of the Mini Stream as a power of 2.
     * The sector size of the Mini Stream MUST be 64 bytes.
     */
    if (hdr.mini_sector_shift != 6)
      return std::unexpected(ole::Error::InvalidMiniSectorShift);

    /**
     * Reserved (6 bytes): This field MUST be set to all zeroes.
     */
    if (constexpr decltype(hdr.reserved) zeroes{}; hdr.reserved != zeroes)
      return std::unexpected(ole::Error::InvalidReservedField);

    /**
     * Number of Directory Sectors (4 bytes):
     * This integer field contains the count of the number of directory sectors in the compound file.
     * If Major Version is 3, the Number of Directory Sectors MUST be zero.
     * This field is not supported for version 3 compound files.
     */
    if (constexpr decltype(hdr.num_dir_sectors) zeroes{}; hdr.major_version == 3 && hdr.num_dir_sectors != zeroes)
      return std::unexpected(ole::Error::InvalidNumberOfDirectorySectors);

    /**
     * Mini Stream Cutoff Size (4 bytes): This integer field MUST be set to 0x00001000.
     * This field specifies the maximum size of a user-defined data stream that is allocated from the mini FAT and mini
     * stream, and that cutoff is 4096 bytes. Any user-defined data stream that is greater than or equal to this cutoff
     * size must be allocated as normal sectors from the FAT.
     */
    if (hdr.mini_stream_cutoff_size != 4096) {
      return std::unexpected(ole::Error::InvalidMiniCutoff);
    }

    /**
     * Утверждения hdr.first_mini_fat_sector != ENDOFCHAIN и hdr.num_mini_fat_sectors != 0
     * должны быть оба True либо оба False.
     */
    {
      const auto exists = hdr.first_mini_fat_sector != ole::ENDOFCHAIN;
      const auto at_least_one = hdr.num_mini_fat_sectors != 0;
      if (const auto valid = exists == at_least_one; not valid) {
        return std::unexpected(ole::Error::MiniFatHeaderInconsistent);
      }
    }

  }

  return hdr;
}

template <bool Validate = true>
std::expected<std::vector<ole::fat_t>, ole::Error> load_fat(ole::FileDevice &device, const ole::OleHeader &header) {
  const auto sector_size = 1 << header.sector_shift;

  // Список sector ID-ов FAT-секторов (строго по csectFat)
  std::vector<ole::fat_t> fat_sector_ids;
  fat_sector_ids.reserve(header.num_fat_sectors);

  /**
   * Непонятно что делать, если в этой цепочке появляется FREESECT.
   * https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-cfb/0afa4e43-b18f-432a-9917-4f276eca7a73
   * Документация не запрещает здесь FREESECT, а после него валидные значения.
   * Зачем так делать непонятно.
   */
  std::ranges::copy_if(header.difat, std::back_inserter(fat_sector_ids),
                       [](auto sector) { return sector != ole::FREESECT; });

  // читаем цепочку DIFAT-секторов
  for (auto next_difat = header.first_difat_sector; next_difat != ole::ENDOFCHAIN;) {
    std::vector<ole::fat_t> buf(sector_size / sizeof(ole::fat_t));
    if (!device.read_at(sector_offset(next_difat, sector_size), as_writable_bytes(std::span{buf}))) [[unlikely]] {
      return std::unexpected(ole::Error::IoFailure);
    }

    // читаем весь сектор
    std::ranges::copy_if(buf, std::back_inserter(fat_sector_ids), [](auto sector) { return sector != ole::FREESECT; });

    // последняя uint32_t запись в секторе - это номер следующего difat сектора
    next_difat = fat_sector_ids.back();
    // удаляем последнюю запись, так как мы хотим хранить только fat сектора, а не служебные difat
    fat_sector_ids.pop_back();
  }

  if (fat_sector_ids.size() != header.num_fat_sectors) {
    return std::unexpected(ole::Error::CorruptedFile);
  }

  // 2) Читаем сами FAT-сектора и склеиваем единый FAT
  std::vector<ole::fat_t> fat;
  fat.reserve(fat_sector_ids.size() * sector_size / sizeof(uint32_t));

  for (auto fat_sid : fat_sector_ids) {
    std::vector<ole::fat_t> buf(sector_size / sizeof(ole::fat_t));
    if (!device.read_at(sector_offset(fat_sid, sector_size), as_writable_bytes(std::span{buf}))) [[unlikely]] {
      return std::unexpected(ole::Error::IoFailure);
    }
    std::ranges::copy(buf, std::back_inserter(fat));
  }

  // std::cout << std::format("number fat sectors: expected: {}, actual: {}", header.num_fat_sectors, size(fat)) << '\n';
  // std::ranges::transform(fat, std::ostream_iterator<std::string>(std::cout, "\n"), [](auto v) -> std::string {
  //   switch (v) {
  //   case ole::FREESECT:
  //     return "ole::FREESECT";
  //   case FATSECT:
  //     return "FATSECT";
  //   case DIFSECT:
  //     return "DIFSECT";
  //   case ole::ENDOFCHAIN:
  //     return "ole::ENDOFCHAIN";
  //   default:
  //     return std::format("{:08X}", v);
  //   }
  // });
  // TODO валидация fat
  return fat;
}

template <bool Validate = true>
std::expected<std::vector<ole::DirectoryEntryRaw>, ole::Error> load_directories(ole::FileDevice &device, const ole::OleHeader &header,
                                                                      const std::vector<ole::fat_t> &fat) {
  const auto sector_size = 1 << header.sector_shift;
  static_assert(sizeof(ole::DirectoryEntryRaw) == 128);
  // идём по FAT-цепочке, начиная с first_dir_sector. Alloc на 32 временно, потом надо посчитать на сколько именно
  // аллокать
  std::vector<ole::DirectoryEntryRaw> dir_stream;
  dir_stream.reserve(header.num_dir_sectors == 0 ? 32 : header.num_dir_sectors);

  // TODO bitcast to wire type
  for (auto next_sector = header.first_dir_sector; next_sector != ole::ENDOFCHAIN; next_sector = fat[next_sector]) {
    // The directory entry size is fixed at 128 bytes.
    std::vector<ole::DirectoryEntryRaw> buf(sector_size / sizeof(ole::DirectoryEntryRaw));
    if (!device.read_at(sector_offset(next_sector, sector_size), as_writable_bytes(std::span{buf}))) [[unlikely]] {
      return std::unexpected(ole::Error::IoFailure);
    }

    std::ranges::move(buf, std::back_inserter(dir_stream));
  }

  for (auto&& dir : dir_stream) {
    std::span<std::byte, sizeof(ole::DirectoryEntryRaw)> view{reinterpret_cast<std::byte *>(std::addressof(dir)), sizeof(ole::DirectoryEntryRaw)};
    ole::DirectoryEntryRaw entry{};
    [[maybe_unused]] const auto tail = view
    | entry.name
    | entry.name_size_in_bytes
    | entry.object_type
    | entry.color_flag
    | entry.left_id
    | entry.right_id
    | entry.child_id
    | entry.clsid
    | entry.state_bits
    | entry.creation_time
    | entry.modified_time
    | entry.starting_sector
    | entry.stream_size;

    assert(empty(tail));
    std::swap(entry, dir);
  }

  std::erase(dir_stream, ole::DirectoryEntryRaw {});
  // TODO directory entry validation

  return dir_stream;
}

template <bool Validate = true>
std::expected<std::vector<ole::fat_t>, ole::Error> load_minifat(ole::FileDevice &device, int sector_size, uint32_t first_mini_fat_sector, uint32_t num_mini_fat_sectors, uint32_t mini_sectors_count, const std::vector<ole::fat_t> &fat) {
  std::vector<ole::fat_t> result; result.reserve(num_mini_fat_sectors);

  // читаем цепочку miniFAT-секторов
  for (auto next_sector = first_mini_fat_sector; next_sector != ole::ENDOFCHAIN; next_sector = fat[next_sector]) {
    std::vector<ole::fat_t> buf(sector_size / sizeof(ole::fat_t));
    if (!device.read_at(sector_offset(next_sector, sector_size), as_writable_bytes(std::span{buf}))) [[unlikely]] {
      return std::unexpected(ole::Error::IoFailure);
    }
    // читаем весь сектор
    std::ranges::copy_if(buf, std::back_inserter(result), [](auto sector) { return sector != ole::FREESECT; });
  }

  if constexpr (Validate) {
    if (result.size() != mini_sectors_count) {
      return std::unexpected(ole::Error::CorruptedFile);
    }
  }

  return result;
}

std::expected<ole::Filesystem, ole::Filesystem::error_type> ole::Filesystem::mount(FileDevice &&dev) {
  auto header = load_header(dev);
  if (not header) {
    return std::unexpected(header.error());
  }

  auto fat = load_fat(dev, *header);
  if (not fat) {
    return std::unexpected(fat.error());
  }

  auto directories = load_directories(dev, *header, *fat);
  if (not directories) {
    return std::unexpected(directories.error());
  }

  auto mini_sectors_count = directories->front().stream_size / (1 << header->mini_sector_shift);
  auto minifat = load_minifat(dev, 1 << header->sector_shift, header->first_mini_fat_sector, header->num_mini_fat_sectors, mini_sectors_count, *fat);
  if (not minifat) {
    return std::unexpected(minifat.error());
  }

  std::vector<ole::DirectoryEntry> dirs; dirs.reserve(directories->size());
  for (const auto& entry : *directories) {
    ole::DirectoryEntry new_entry {};

    auto name = ole::String::make(entry.name, entry.name_size_in_bytes);
    if (not name) {
      return std::unexpected(name.error());
    }

    new_entry.type = static_cast<ole::file_type>(entry.object_type);
    new_entry.name = std::move(*name);
    new_entry.left_id = entry.left_id;
    new_entry.right_id = entry.right_id;
    new_entry.child_id = entry.child_id;
    new_entry.starting_sector = entry.starting_sector;
    new_entry.creation_time = entry.creation_time;
    new_entry.modified_time = entry.modified_time;
    new_entry.stream_size = entry.stream_size;

    dirs.push_back(std::move(new_entry));
  }

  // auto ministream = load_ministream(dev);
  // if (not ministream) {
  //   return std::unexpected(ministream.error());
  // }

  Filesystem driver{std::move(dev)};
  driver.dirs_ = std::move(dirs);

  return driver;
}

[[nodiscard]] bool ole::Filesystem::exists(const ole::Path& path) const noexcept {
  using namespace std::ranges;
  std::size_t root = dirs_.front().child_id;

  ole::Path result;
  for (auto[entry, _] : views::zip(PathResolve(dirs_, path, root), path)) {
    result.append(entry.name);
  }

  return !result.empty() && result == path;
}

[[nodiscard]] std::expected<int, ole::Filesystem::error_type> ole::Filesystem::file_size(const Path& path) const noexcept {
  using namespace std::ranges;

  if (empty(path)) [[unlikely]] {
    return std::unexpected(Error::FileNotFound);
  }

  std::size_t root = dirs_.front().child_id;

  Path result;
  auto found = std::cref(dirs_.front());
  for (auto[entry, _] : views::zip(PathResolve(dirs_, path, root), path)) {
    result.append(entry.name);
    found = std::cref(entry);
  }

  if (result != path) {
    return std::unexpected(Error::FileNotFound);
  }

  if (found.get().type != file_type::regular) {
    return std::unexpected(Error::NotRegularFile);
  }

  return found.get().stream_size;
}

[[nodiscard]] bool ole::Filesystem::is_directory(const Path& path) const noexcept { return false; }
[[nodiscard]] bool ole::Filesystem::is_regular_file(const Path& path) const noexcept { return false; }

ole::Filesystem::Filesystem(FileDevice &&dev) : dev_{std::move(dev)} {}