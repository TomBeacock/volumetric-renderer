#pragma once

#include <glm/glm.hpp>

#include <vector>

namespace Vol::Data
{
struct Dataset {
    glm::u32vec3 dimensions;
    std::vector<float> data;
};
}  // namespace Vol::Data