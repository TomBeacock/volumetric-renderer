#include "csv_file_parser.h"

#include <glm/glm.hpp>

#include <fstream>
#include <sstream>

Vol::Data::CsvFileParser::CsvFileParser(
    const std::vector<std::filesystem::path> &filepaths)
    : MultiFileParser(filepaths)
{
}

Vol::Data::Dataset Vol::Data::CsvFileParser::parse()
{
    Dataset dataset{};

    std::string line, value_str;
    uint32_t x = 0, y = 0, z = 0;
    for (const std::filesystem::path &filepath : get_filepaths()) {
        std::ifstream file(filepath);
        y = 0;
        while (std::getline(file, line)) {
            std::stringstream ss(line);
            x = 0;
            while (std::getline(ss, value_str, ',')) {
                float value = std::stof(value_str);
                dataset.min = std::min(dataset.min, value);
                dataset.max = std::max(dataset.max, value);
                dataset.data.push_back(value);
                x++;
            }
            if (z == 0 && y == 0) {
                dataset.dimensions.x = x;
            } else if (x != dataset.dimensions.x) {
                throw std::runtime_error("Inconsistant dimensions");
            }
            y++;
        }
        if (z == 0) {
            dataset.dimensions.y = y;
        } else if (y != dataset.dimensions.y) {
            throw std::runtime_error("Inconsistant dimensions");
        }
        z++;
        file.close();
    }
    dataset.dimensions.z = z;
    return dataset;
}
