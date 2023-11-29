#include "slider.h"

#include <imgui.h>

void Vol::UI::Components::slider(
    const std::string &label,
    float *value,
    float min,
    float max,
    const std::string &status,
    std::string &status_text)
{
    ImGui::TableNextRow();

    ImGui::TableNextColumn();
    ImGui::AlignTextToFramePadding();
    ImGui::Text(label.c_str());

    ImGui::TableNextColumn();
    float slider_width = ImGui::GetContentRegionAvail().x -
                         ImGui::GetStyle().ItemSpacing.x - 54.0f;
    ImGui::PushItemWidth(slider_width);
    std::string slider_label = "##" + label + "_slider";
    ImGui::SliderFloat(slider_label.c_str(), value, min, max, "");
    if (ImGui::IsItemHovered()) {
        status_text = status;
    }

    ImGui::SameLine();
    ImGui::PushItemWidth(0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1);
    std::string input_label = "##" + label + "_input";
    ImGui::InputFloat(input_label.c_str(), value, 0.0f, 0.0f, "%.1f");
    if (ImGui::IsItemHovered()) {
        status_text = status;
    }
    ImGui::PopStyleVar();
}
