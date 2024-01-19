#pragma once

#include <filesystem>

namespace Vol::Data
{
class FileParser;
}  // namespace Vol::Data

namespace Vol::Data
{
enum class FileFormat {
    Nrrd,
};

class Importer {
  public:
    void import(FileFormat file_format) const;
};
}  // namespace Vol::Data