#include "image_rounded.h"

#include <imgui.h>
#include <imgui_internal.h>

void Vol::UI::Components::image_rounded(
    ImTextureID user_texture_id,
    const ImVec2 &size,
    const ImVec2 &uv0,
    const ImVec2 &uv1,
    const ImVec4 &tint_col,
    const ImVec4 &border_col,
    float rounding)
{
    ImGuiWindow *window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return;

    ImRect bb(
        window->DC.CursorPos,
        ImVec2(
            window->DC.CursorPos.x + size.x, window->DC.CursorPos.y + size.y));
    if (border_col.w > 0.0f) {
        bb.Max = ImVec2(bb.Max.x + 2, bb.Max.y + 2);
    }
    ImGui::ItemSize(bb);
    if (!ImGui::ItemAdd(bb, 0))
        return;

    if (border_col.w > 0.0f) {
        window->DrawList->AddRect(
            bb.Min, bb.Max, ImGui::GetColorU32(border_col), 0.0f);
        window->DrawList->AddImageRounded(
            user_texture_id, ImVec2(bb.Min.x + 1, bb.Min.y + 1),
            ImVec2(bb.Max.x - 1, bb.Max.y - 1), uv0, uv1,
            ImGui::GetColorU32(tint_col), rounding);
    } else {
        window->DrawList->AddImageRounded(
            user_texture_id, bb.Min, bb.Max, uv0, uv1,
            ImGui::GetColorU32(tint_col), rounding);
    }
}
