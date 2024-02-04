#pragma once

#include <string>

namespace Vol::UI::Components
{
bool attribute_float(
    const std::string &label,
    float *value,
    float min,
    float max,
    const std::string &status,
    std::string &status_text,
    const char *format = "%.1f");

bool attribute_float_range(
    const std::string &label,
    float *value_min,
    float *value_max,
    float min,
    float max,
    const std::string &status,
    std::string &status_text,
    const char *format = "%.1f");
}  // namespace Vol::UI::Components