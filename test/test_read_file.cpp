#include "containerfs/ole.h"
#include "containerfs/filesystem.h"

#include <gtest/gtest.h>
#include <vector>
#include <iostream>

namespace ole {
template<typename T>
std::ostream& operator<<(std::ostream& os, T t) requires (std::is_enum_v<T>) {
  return os << static_cast<std::streamsize>(t);
}
}

using namespace containerfs;

std::vector<std::byte> read_file(const std::filesystem::path& path) {
  std::ifstream file{path, std::ios::binary | std::ios::in};
  std::vector<std::byte> result;
  result.resize(std::filesystem::file_size(path));
  file.read(reinterpret_cast<char*>(result.data()), result.size());

  return result;
}

TEST(Exists, Positive) {
  using namespace std::filesystem;

  const path file{"exists.ole"};
  EXPECT_TRUE(exists(file));
  FileDevice dev{file};

  auto fs = mount<OleDriver>(std::move(dev));
  EXPECT_TRUE(fs) << fs.error();

  for (auto&& dir_entry : recursive_directory_iterator("exists")) {
    auto p = ole::Path::make(dir_entry.path());
    EXPECT_TRUE(p) << p.error();
    EXPECT_TRUE(dir_entry.exists()) << dir_entry;
    EXPECT_TRUE(fs->exists(*p)) << *p;
  }
}

TEST(Exists, Negative) {
  using namespace std::filesystem;

  const path file{"exists.ole"};
  EXPECT_TRUE(exists(file));
  FileDevice dev{file};

  auto fs = mount<OleDriver>(std::move(dev));
  EXPECT_TRUE(fs) << fs.error();

  {
    auto p = ole::Path::make("");
    EXPECT_TRUE(p) << p.error();
    EXPECT_FALSE(fs->exists(*p)) << *p;
  }

  for (auto&& dir_entry : recursive_directory_iterator("exists")) {
    auto p = ole::Path::make(dir_entry.path() / "nonexistent_path");
    EXPECT_TRUE(p) << p.error();
    EXPECT_TRUE(dir_entry.exists()) << dir_entry;
    EXPECT_FALSE(fs->exists(*p)) << *p;
  }
}

TEST(OLETest, DISABLED_ReadSmallFile) {
  const std::filesystem::path file{"test.ole"};
  EXPECT_TRUE(exists(file));
  FileDevice dev{file};

  auto fs = mount<OleDriver>(std::move(dev));
  if (not fs) {
    EXPECT_EQ(static_cast<int>(fs.error()), -1);
  }

  auto sz = sizeof(fs);
  std::cout << "sizeof(fs) = " << sz << std::endl;
  for (auto&& dir_entry :
            std::filesystem::recursive_directory_iterator("root")) {
    // 2. Читаем содержимое файла "a.txt"
    if (dir_entry.is_directory()) {
      std::filesystem::exists(dir_entry.path());
      // EXPECT_TRUE(fs->is_directory(dir_entry));
      auto p = ole::Path::make(dir_entry.path());
      if (not p) {
        EXPECT_EQ(static_cast<int>(p.error()), -1);
      }

      EXPECT_EQ(dir_entry.exists(), fs->exists(*p));
    } else {
      // EXPECT_EQ(dir_entry.file_size(), fs->file_size(dir_entry));
      // EXPECT_EQ(read_file(dir_entry), fs->read_file(dir_entry));
    }
  }
}
