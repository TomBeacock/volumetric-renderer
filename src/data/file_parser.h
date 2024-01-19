#pragma once

#include "dataset.h"

#include <glm/glm.hpp>

#include <filesystem>
#include <string>

namespace Vol::Data
{
class FileParser {
  public:
    virtual Dataset parse() = 0;
};

class SingleFileParser : public FileParser {
  public:
    SingleFileParser(const std::filesystem::path &filepath)
        : filepath(filepath){};

  protected:
    const std::filesystem::path &get_filepath() const { return filepath; };

  private:
    std::filesystem::path filepath;
};

class MultiFileParser : public FileParser {
  public:
    MultiFileParser(const std::vector<std::filesystem::path> &filepaths)
        : filepaths(filepaths){};

  protected:
    const std::vector<std::filesystem::path> get_filepaths() const
    {
        return filepaths;
    }

  private:
    std::vector<std::filesystem::path> filepaths;
};
}  // namespace Vol::Data