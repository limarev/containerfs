#include "containerfs/ole.h"
// #include "containerfs/ole.h"
// #include <vector>
//
// struct CFHeader {
//   std::array<std::byte, 8> HeaderSignature;
//   std::array<std::byte, 16> HeaderCLSID;
//   std::array<std::byte, 2> MinorVersion;
//   std::array<std::byte, 2> MajorVersion;
//   std::array<std::byte, 2> ByteOrder;
//   std::array<std::byte, 2> SectorShift;
//   std::array<std::byte, 2> MiniSectorShift;
//   std::array<std::byte, 6> Reserved;
//   std::array<std::byte, 4> NumberOfDirectorySectors;
//   std::array<std::byte, 4> NumberOfFATSectors;
//   std::array<std::byte, 4> FirstDirectorySectorLocation;
//   std::array<std::byte, 4> TransactionSignatureNumber;
//   std::array<std::byte, 4> MiniStreamCutoffSize;
//   std::array<std::byte, 4> FirstMiniFATSectorLocation;
//   std::array<std::byte, 4> NumberOfMiniFATSectors;
//   std::array<std::byte, 4> FirstDIFATSectorLocation;
//   std::array<std::byte, 4> NumberOfDIFATSectors;
//   std::array<std::byte, 436> DIFAT;
// };
//
// template <size_t Size> std::istream &operator>>(std::istream &is, std::array<std::byte, Size> buffer) {
//   is.read(reinterpret_cast<std::istream::char_type *>(std::data(buffer)), Size);
//   return is;
// }
//
// std::istream &operator>>(std::istream &is, CFHeader &header) {
//   is >> header.HeaderSignature;
//   is >> header.HeaderCLSID;
//   is >> header.MinorVersion;
//   is >> header.MajorVersion;
//   is >> header.ByteOrder;
//   is >> header.SectorShift;
//   is >> header.MiniSectorShift;
//   is >> header.Reserved;
//   is >> header.NumberOfDirectorySectors;
//   is >> header.NumberOfFATSectors;
//   is >> header.FirstDirectorySectorLocation;
//   is >> header.TransactionSignatureNumber;
//   is >> header.MiniStreamCutoffSize;
//   is >> header.FirstMiniFATSectorLocation;
//   is >> header.NumberOfMiniFATSectors;
//   is >> header.FirstDIFATSectorLocation;
//   is >> header.NumberOfDIFATSectors;
//   is >> header.DIFAT;
//
//   return is;
// }
//
// bool validate(const CFHeader &header) {
//   // if (header.HeaderSignature != {'0xD0', '0xCF', '0x11', '0xE0', '0xA1', '0xB1', '0x1A', '0xE1'})
//   // return false;
//   return false;
// }
//
// ole::OLEFStream::OLEFStream(std::filesystem::path path) : of_(path) {
//   CFHeader header;
//   of_ >> header;
//   auto success = validate(header);
// }
//
// void ole::OLEFStream::write(std::string, std::vector<uint8_t> v) {
// }
//
// void ole::OLEFStream::read() {
// }
