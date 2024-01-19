#pragma once

#include <string>

namespace Vol::UI::Components
{
void slider(
    const std::string &label,
    float *value,
    float min,
    float max,
    const std::string &status,
    std::string &status_text);
}