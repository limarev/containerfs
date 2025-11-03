#include "path.h"

std::expected<ole::Path, ole::Error> ole::Path::make(const std::filesystem::path &path) {
  std::vector<String> paths;
  // TODO заменить 32 на что-то более разумное
  paths.reserve(32);
  for (const auto& p: path) {
    auto result = String::make(p);
    if (not result) {
      return std::unexpected(result.error());
    }

    paths.push_back(*result);
  }

  return Path(std::move(paths));
}

ole::Path::Path(std::vector<String> paths): paths_(std::move(paths)) {}