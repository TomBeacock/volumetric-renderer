#pragma once

#include "dataset.h"

#include <glm/glm.hpp>

#include <filesystem>
#include <string>

namespace Vol::Data
{
class FileParser {
  public:
    FileParser(const std::filesystem::path &filepath);

    virtual Dataset parse() = 0;

  protected:
    const std::filesystem::path filepath;
};

class NrrdFileParser : public FileParser {
  public:
    explicit NrrdFileParser(const std::filesystem::path &filepath);

    virtual Dataset parse() override;
};
}  // namespace Vol::Data