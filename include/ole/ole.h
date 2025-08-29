#pragma once

#include <filesystem>
#include <fstream>
#include <vector>

// interface

namespace ole {

class OLEFStream {
public:
  OLEFStream(std::filesystem::path path);
  void write(std::string, std::vector<uint8_t> v);
  void read();

private:
  std::fstream of_;
};

} // namespace ole
