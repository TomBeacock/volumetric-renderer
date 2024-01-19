#pragma once

#include "file_parser.h"

namespace Vol::Data
{
class CsvFileParser : public MultiFileParser {
  public:
    CsvFileParser(const std::vector<std::filesystem::path> &filepaths);

    virtual Dataset parse() override;
};
}  // namespace Vol::Data