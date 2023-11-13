#pragma once

#include <filesystem>

namespace Vol::Data
{
class FileParser;
}  // namespace Vol::Data

namespace Vol::Data
{
class Importer {
  public:
    void import(const std::filesystem::path &filepath) const;
};
}  // namespace Vol::Data