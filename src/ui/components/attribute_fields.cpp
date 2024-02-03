#include "attribute_fields.h"

#include "slider.h"

#include <imgui.h>

#include <algorithm>

constexpr float input_field_width = 54.0f;

void Vol::UI::Components::attribute_float(
    const std::string &label,
    float *value,
    float min,
    float max,
    const std::string &status,
    std::string &status_text,
    const char *format)
{
    ImGui::TableNextRow();

    ImGui::TableNextColumn();
    ImGui::AlignTextToFramePadding();
    ImGui::Text(label.c_str());

    ImGui::TableNextColumn();
    float slider_width = ImGui::GetContentRegionAvail().x -
                         ImGui::GetStyle().ItemSpacing.x - input_field_width;
    ImGui::SetNextItemWidth(slider_width);
    std::string slider_label = "##" + label + "_slider";
    slider_float(
        slider_label.c_str(), value, min, max, "", ImGuiSliderFlags_NoInput);
    if (ImGui::IsItemHovered()) {
        status_text = status;
    }

    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(input_field_width);
    std::string input_label = "##" + label + "_input";
    ImGui::InputFloat(input_label.c_str(), value, 0.0f, 0.0f, format);
    if (ImGui::IsItemHovered()) {
        status_text = status;
    }
    ImGui::PopStyleVar();

    *value = std::clamp(*value, min, max);
}

void Vol::UI::Components::attribute_float_range(
    const std::string &label,
    float *value_min,
    float *value_max,
    float min,
    float max,
    const std::string &status,
    std::string &status_text,
    const char *format)
{
    ImGui::TableNextRow();

    ImGui::TableNextColumn();
    ImGui::AlignTextToFramePadding();
    ImGui::Text(label.c_str());

    ImGui::TableNextColumn();

    float slider_width =
        ImGui::GetContentRegionAvail().x -
        (ImGui::GetStyle().ItemSpacing.x + input_field_width) * 2;
    ImGui::SetNextItemWidth(slider_width);
    std::string slider_label = "##" + label + "_slider";
    range_slider_float(
        slider_label.c_str(), value_min, value_max, min, max, "",
        ImGuiSliderFlags_NoInput);
    if (ImGui::IsItemHovered()) {
        status_text = status;
    }

    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1);

    ImGui::SameLine();
    ImGui::SetNextItemWidth(input_field_width);
    std::string input_label_min = "##" + label + "_input_min";
    ImGui::InputFloat(input_label_min.c_str(), value_min, 0.0f, 0.0f, format);
    if (ImGui::IsItemHovered()) {
        status_text = status;
    }

    ImGui::SameLine();
    ImGui::SetNextItemWidth(input_field_width);
    std::string input_label_max = "##" + label + "_input_max";
    ImGui::InputFloat(input_label_max.c_str(), value_max, 0.0f, 0.0f, format);
    if (ImGui::IsItemHovered()) {
        status_text = status;
    }

    ImGui::PopStyleVar();

    *value_min = std::clamp(*value_min, min, max);
    *value_max = std::clamp(*value_max, min, max);
}
