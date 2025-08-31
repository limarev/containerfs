#include "containerfs/ole.h"
#include "containerfs/filesystem.h"

#include <gtest/gtest.h>
#include <vector>

using namespace containerfs;

std::vector<std::byte> read_file(const std::filesystem::path& path) {
  std::ifstream file{path, std::ios::binary | std::ios::in};
  std::vector<std::byte> result;
  result.resize(std::filesystem::file_size(path));
  file.read(reinterpret_cast<char*>(result.data()), result.size());

  return result;
}

TEST(OLETest, ReadSmallFile) {
  FileDevice dev{"test.ole"};
  auto fs = mount<OleDriver>(std::move(dev));
  EXPECT_TRUE(fs);
  auto sz = sizeof(fs);
  std::cout << "sizeof(fs) = " << sz << std::endl;
  for (auto&& dir_entry :
            std::filesystem::recursive_directory_iterator("root")) {
    // 2. Читаем содержимое файла "a.txt"
    if (dir_entry.is_directory()) {
      std::filesystem::exists(dir_entry.path());
      EXPECT_TRUE(fs->is_directory(dir_entry));
      EXPECT_EQ(dir_entry.exists(), fs->exists(dir_entry));
    } else {
      EXPECT_EQ(dir_entry.file_size(), fs->file_size(dir_entry));
      EXPECT_EQ(read_file(dir_entry), fs->read_file(dir_entry));
    }
  }
}
