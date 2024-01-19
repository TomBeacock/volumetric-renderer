#pragma once

#include <imgui.h>

namespace Vol::UI::Components
{
void image_rounded(
    ImTextureID user_texture_id,
    const ImVec2 &size,
    const ImVec2 &uv0 = ImVec2(0, 0),
    const ImVec2 &uv1 = ImVec2(1, 1),
    const ImVec4 &tint_col = ImVec4(1, 1, 1, 1),
    const ImVec4 &border_col = ImVec4(0, 0, 0, 0),
    float rounding = 0.0f);
}