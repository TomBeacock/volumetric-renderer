#include "file_parser.h"

#include <NrrdIO.h>
#include <glm/glm.hpp>

#include <exception>
#include <iostream>
#include <map>
#include <string>

std::vector<float> convert(void *data, int nrrd_type, glm::u32vec3 dimensions);

template <typename T>
std::vector<float> convert(void *data, size_t size);

Vol::Data::FileParser::FileParser(const std::filesystem::path &filepath)
    : filepath(filepath)
{
}

Vol::Data::NrrdFileParser::NrrdFileParser(const std::filesystem::path &filepath)
    : FileParser(filepath)
{
}

Vol::Data::Dataset Vol::Data::NrrdFileParser::parse()
{
    Nrrd *nrrd_file = nrrdNew();
    if (nrrdLoad(nrrd_file, filepath.string().c_str(), nullptr)) {
        throw std::runtime_error("Failed to read file");
    }

    if (nrrd_file->dim != 3) {
        throw std::runtime_error("Invalid file properties");
    }

    NrrdAxisInfo *axis = nrrd_file->axis;
    glm::u32vec3 dimensions(axis[0].size, axis[1].size, axis[2].size);

    std::vector<float> data =
        convert(nrrd_file->data, nrrd_file->type, dimensions);
    Dataset dataset{
        .dimensions = dimensions,
        .min = *std::min_element(data.begin(), data.end()),
        .max = *std::max_element(data.begin(), data.end()),
        .data = data,
    };

    nrrdNuke(nrrd_file);

    return dataset;
}

std::vector<float> convert(void *data, int nrrd_type, glm::u32vec3 dimensions)
{
    size_t size = (size_t)dimensions.x * dimensions.y * dimensions.z;
    switch (nrrd_type) {
        case nrrdTypeChar: return convert<int8_t>(data, size);
        case nrrdTypeUChar: return convert<uint8_t>(data, size);
        case nrrdTypeShort: return convert<int16_t>(data, size);
        case nrrdTypeUShort: return convert<uint16_t>(data, size);
        case nrrdTypeInt: return convert<int32_t>(data, size);
        case nrrdTypeUInt: return convert<uint32_t>(data, size);
        case nrrdTypeLLong: return convert<int64_t>(data, size);
        case nrrdTypeULLong: return convert<uint64_t>(data, size);
        case nrrdTypeFloat: return convert<float>(data, size);
        case nrrdTypeDouble: return convert<double>(data, size);
    }
    return std::vector<float>();
}

template <typename T>
std::vector<float> convert(void *data, size_t size)
{
    T *d = static_cast<T *>(data);
    std::vector<float> result(size);

    for (size_t i = 0; i < size; i++) {
        result[i] = static_cast<float>(d[i]);
    }
    return result;
}
