#pragma once

#include "file_parser.h"

namespace Vol::Data
{
class NrrdFileParser : public SingleFileParser {
  public:
    explicit NrrdFileParser(const std::filesystem::path &filepath);

    virtual Dataset parse() override;
};
}  // namespace Vol::Data