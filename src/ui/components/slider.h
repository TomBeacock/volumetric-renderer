#pragma once

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif

#include <imgui.h>

#include <string>

namespace Vol::UI::Components
{
bool slider_float(
    const char *label,
    float *value,
    float min,
    float max,
    const char *format = "%.3f",
    ImGuiSliderFlags flags = 0);

bool range_slider_float(
    const char *label,
    float *value_min,
    float *value_max,
    float min,
    float max,
    const char *format = "%.3f",
    ImGuiSliderFlags flags = 0);
}  // namespace Vol::UI::Components