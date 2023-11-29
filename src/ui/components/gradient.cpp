#include "gradient.h"

#include <imgui.h>
#include <imgui_internal.h>

#include <algorithm>
#include <cmath>
#include <format>
#include <set>

using namespace Vol::UI::Components;

template <typename T>
static T lerp(T a, T b, float t)
{
    return a * (1.0f - t) + b * t;
}

template <typename T>
T sample_markers(std::vector<Gradient::Marker<T>> &markers, float location);

template <typename T>
std::pair<bool, size_t> add_marker(
    std::vector<Gradient::Marker<T>> &markers,
    float location,
    const T &value);

template <typename T>
bool remove_marker(std::vector<T> &markers, size_t index);

template <typename T, typename F>
std::pair<bool, bool> update_markers(
    const char *str_id,
    std::vector<T> &markers,
    GradientEditState &state,
    GradientEditState::MarkerType type,
    const ImVec2 &min,
    const ImVec2 &max,
    float marker_radius,
    F &&get_color);

template <typename T, typename F>
void update_marker(
    const char *str_id,
    std::vector<T> &markers,
    int index,
    GradientEditState &state,
    GradientEditState::MarkerType type,
    const ImVec2 &min,
    const ImVec2 &max,
    float marker_radius,
    F &&get_color,
    bool &changed,
    bool &hovered,
    int &selected_index);

void draw_marker(
    const ImVec2 &start,
    const ImVec2 &end,
    float radius,
    const ImU32 &color,
    bool selected = false);

Vol::UI::Components::Gradient::Gradient()
    : color_markers(
          {{0.0f, glm::vec3(0.0f, 0.0f, 0.0f)},
           {1.0f, glm::vec3(1.0f, 1.0f, 1.0f)}}),
      alpha_markers({{0.0f, 1.0f}, {1.0f, 1.0f}})
{
}

glm::vec3 Vol::UI::Components::Gradient::sample_color(float location)
{
    return sample_markers(color_markers, location);
}

float Vol::UI::Components::Gradient::sample_alpha(float location)
{
    return sample_markers(alpha_markers, location);
}

glm::vec4 Vol::UI::Components::Gradient::sample(float location)
{
    glm::vec3 color = sample_color(location);
    float alpha = sample_alpha(location);

    return glm::vec4(color.r, color.g, color.b, alpha);
}

std::vector<glm::vec4> Vol::UI::Components::Gradient::discretize(size_t count)
{
    float stride = 1.0f / count;
    float offset = stride / 2.0f;

    std::vector<glm::vec4> discrete;
    discrete.reserve(count);

    float location = offset;
    for (size_t i = 0; i < count; i++) {
        discrete.push_back(sample(location));
        location += stride;
    }

    return discrete;
}

std::pair<bool, size_t> Vol::UI::Components::Gradient::add_color_marker(
    float location,
    glm::vec3 value)
{
    return add_marker(color_markers, location, value);
}

std::pair<bool, size_t> Vol::UI::Components::Gradient::add_alpha_marker(
    float location,
    float value)
{
    return add_marker(alpha_markers, location, value);
}

bool Vol::UI::Components::Gradient::remove_color_marker(size_t index)
{
    return remove_marker(color_markers, index);
}

bool Vol::UI::Components::Gradient::remove_alpha_marker(size_t index)
{
    return remove_marker(alpha_markers, index);
}

bool Vol::UI::Components::gradient_edit(
    const char *str_id,
    Gradient &gradient,
    GradientEditState &state,
    std::string &status_text)
{
    bool changed = false;

    ImGui::PushID(str_id);

    ImDrawList *draw_list = ImGui::GetWindowDrawList();

    float marker_radius = 6.0f;
    float marker_height = 16.0f;
    float width = ImGui::GetContentRegionAvail().x;
    float inner_width = width - marker_radius * 2.0f;

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));

    // Alpha markers
    {
        ImVec2 alpha_origin = ImGui::GetCursorScreenPos();
        alpha_origin.x += marker_radius;

        auto [marker_changed, marker_hovered] = update_markers(
            "alphas", gradient.alpha_markers, state,
            GradientEditState::MarkerType::Alpha, alpha_origin,
            ImVec2(
                alpha_origin.x + inner_width, alpha_origin.y + marker_height),
            marker_radius, [](const Gradient::AlphaMarker &marker) -> ImU32 {
                return ImGui::ColorConvertFloat4ToU32(
                    ImVec4(marker.value, marker.value, marker.value, 1.0f));
            });

        changed |= marker_changed;

        ImGui::InvisibleButton(
            "alpha_marker_region", ImVec2(width, marker_height));

        if (!marker_hovered && ImGui::IsItemHovered()) {
            float offset = ImGui::GetMousePos().x - alpha_origin.x;
            float location = offset / inner_width;
            float alpha = gradient.sample_alpha(location);

            float x = alpha_origin.x + offset;

            draw_marker(
                ImVec2(x, alpha_origin.y + marker_height),
                ImVec2(x, alpha_origin.y + marker_radius), marker_radius,
                ImGui::ColorConvertFloat4ToU32(
                    ImVec4(alpha, alpha, alpha, 0.5f)));

            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                auto [marker_changed, index] =
                    gradient.add_alpha_marker(location, alpha);
                changed |= marker_changed;
                state.selected_index = index;
                state.selected_type = GradientEditState::MarkerType::Alpha;
            }
        }
    }

    // Gradient bar
    {
        ImVec2 bar_origin = ImGui::GetCursorScreenPos();
        bar_origin.x += marker_radius;
        ImVec2 bar_size(inner_width, 32.0f);

        // Draw bar background
        ImGui::Dummy(ImVec2(width, bar_size.y));

        draw_list->AddRectFilled(
            bar_origin,
            ImVec2(bar_origin.x + bar_size.x, bar_origin.y + bar_size.y),
            IM_COL32(100, 100, 100, 255));

        // Draw bar grid
        ImVec2 cell_size(8.0f, 8.0f);
        ImVec2 actual_cell_size = cell_size;
        for (size_t y = 0; y * cell_size.y < bar_size.y; y++) {
            actual_cell_size.y =
                std::min(cell_size.y, bar_size.y - y * cell_size.y);

            for (size_t x = 0; x * cell_size.x < bar_size.x; x++) {
                if ((x + y) & 1) {
                    continue;
                }
                actual_cell_size.x =
                    std::min(cell_size.x, bar_size.x - x * cell_size.x);

                ImVec2 cell_pos(
                    bar_origin.x + x * cell_size.x,
                    bar_origin.y + y * cell_size.y);

                draw_list->AddRectFilled(
                    cell_pos,
                    ImVec2(
                        cell_pos.x + actual_cell_size.x,
                        cell_pos.y + actual_cell_size.y),
                    IM_COL32(50, 50, 50, 255));
            }
        }

        // Draw bar gradient
        std::set<float> locations;
        for (Gradient::AlphaMarker &marker : gradient.alpha_markers) {
            locations.insert(marker.location);
        }
        for (Gradient::ColorMarker &marker : gradient.color_markers) {
            locations.insert(marker.location);
        }

        auto end = std::prev(locations.end());
        for (auto it = locations.begin(); it != end; it++) {
            float t1 = *it, t2 = *std::next(it);

            glm::vec4 c1 = gradient.sample(t1);
            glm::vec4 c2 = gradient.sample(t2);

            ImU32 c1_u32 =
                ImGui::ColorConvertFloat4ToU32(ImVec4(c1.r, c1.g, c1.b, c1.a));
            ImU32 c2_u32 =
                ImGui::ColorConvertFloat4ToU32(ImVec4(c2.r, c2.g, c2.b, c2.a));

            draw_list->AddRectFilledMultiColor(
                ImVec2(bar_origin.x + t1 * bar_size.x, bar_origin.y),
                ImVec2(
                    bar_origin.x + t2 * bar_size.x, bar_origin.y + bar_size.y),
                c1_u32, c2_u32, c2_u32, c1_u32);
        }
    }

    ImGui::PopStyleVar();

    // Color markers
    {
        ImVec2 color_origin = ImGui::GetCursorScreenPos();
        color_origin.x += marker_radius;

        auto [marker_changed, marker_hovered] = update_markers(
            "colors", gradient.color_markers, state,
            GradientEditState::MarkerType::Color, color_origin,
            ImVec2(
                color_origin.x + inner_width, color_origin.y + marker_height),
            marker_radius, [](const Gradient::ColorMarker &marker) -> ImU32 {
                return ImGui::ColorConvertFloat4ToU32(ImVec4(
                    marker.value.r, marker.value.g, marker.value.b, 1.0f));
            });

        changed |= marker_changed;

        ImGui::InvisibleButton(
            "color_marker_region", ImVec2(width, marker_height));

        if (!marker_hovered && !state.dragging && ImGui::IsItemHovered()) {
            float offset = ImGui::GetMousePos().x - color_origin.x;
            float location = offset / inner_width;
            glm::vec3 color = gradient.sample_color(location);

            float x = color_origin.x + offset;

            draw_marker(
                ImVec2(x, color_origin.y),
                ImVec2(x, color_origin.y + marker_height - marker_radius),
                marker_radius,
                ImGui::ColorConvertFloat4ToU32(
                    ImVec4(color.r, color.g, color.b, 0.5f)));

            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                auto [marker_changed, index] =
                    gradient.add_color_marker(location, color);
                changed |= marker_changed;
                state.selected_index = index;
                state.selected_type = GradientEditState::MarkerType::Color;
            }
        }
    }

    // Controls
    if (ImGui::BeginTable("gradient_controls", 5)) {
        ImGui::TableSetupColumn(
            "type", ImGuiTableColumnFlags_WidthFixed,
            ImGui::CalcTextSize("Opacity").x);
        ImGui::TableSetupColumn(
            "value", ImGuiTableColumnFlags_WidthFixed, 50.0f);
        ImGui::TableSetupColumn(
            "location", ImGuiTableColumnFlags_WidthFixed,
            ImGui::CalcTextSize("Location").x);
        ImGui::TableSetupColumn(
            "value", ImGuiTableColumnFlags_WidthFixed, 50.0f);
        ImGui::TableSetupColumn("delete", ImGuiTableColumnFlags_WidthFixed);

        bool disable =
            state.selected_type == GradientEditState::MarkerType::None;

        if (disable) {
            ImGui::BeginDisabled();
        }

        ImGui::TableNextRow();

        ImGui::TableNextColumn();
        ImGui::AlignTextToFramePadding();
        switch (state.selected_type) {
            case GradientEditState::MarkerType::Alpha: {
                ImGui::Text("Opacity");

                ImGui::TableNextColumn();
                float alpha =
                    gradient.alpha_markers[state.selected_index].value;
                alpha *= 100.0f;
                ImGui::PushItemWidth(-1.0f);
                ImGui::DragFloat(
                    "##opacity_drag", &alpha, 0.5f, 0.0f, 100.0f, "%.0f%%",
                    ImGuiSliderFlags_AlwaysClamp);
                alpha /= 100.0f;
                gradient.alpha_markers[state.selected_index].value = alpha;
                break;
            }
            case GradientEditState::MarkerType::Color: {
                ImGui::Text("Color");

                ImGui::TableNextColumn();
                glm::vec3 *color =
                    &gradient.color_markers[state.selected_index].value;
                if (ImGui::ColorButton(
                        "##color_edit",
                        ImVec4(color->r, color->g, color->b, 1.0f),
                        ImGuiColorEditFlags_NoTooltip,
                        ImVec2(ImGui::GetColumnWidth(), 0.0f))) {
                    ImGui::OpenPopup("picker-popup");
                }
                if (ImGui::BeginPopup("picker-popup")) {
                    ImGui::ColorPicker3(
                        "##color_picker", &color->r,
                        ImGuiColorEditFlags_NoLabel);
                    ImGui::EndPopup();
                }
                break;
            }
            case GradientEditState::MarkerType::None: {
                ImGui::Text("Opacity");

                ImGui::TableNextColumn();
                float opacity = 0.0f;
                ImGui::PushItemWidth(-1.0f);
                ImGui::DragFloat(
                    "##opacity_drag", &opacity, 0.5f, 0.0f, 100.0f, "%.0f%%",
                    ImGuiSliderFlags_AlwaysClamp);
                break;
            }
        }

        bool locked = false;
        float location = 0.0f;
        switch (state.selected_type) {
            case GradientEditState::MarkerType::Alpha: {
                location =
                    gradient.alpha_markers[state.selected_index].location;
                locked |=
                    state.selected_index == 0 ||
                    state.selected_index == gradient.alpha_markers.size() - 1;
                break;
            }
            case GradientEditState::MarkerType::Color: {
                location =
                    gradient.color_markers[state.selected_index].location;
                locked |=
                    state.selected_index == 0 ||
                    state.selected_index == gradient.color_markers.size() - 1;
                break;
            }
        }

        if (locked) {
            ImGui::BeginDisabled();
        }

        ImGui::TableNextColumn();
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Location");

        ImGui::TableNextColumn();
        location *= 100.0f;
        ImGui::PushItemWidth(-1.0f);
        ImGui::DragFloat(
            "##location_drag", &location, 0.5f, 0.0f, 100.0f, "%.0f%%",
            ImGuiSliderFlags_AlwaysClamp);
        location /= 100.0f;

        switch (state.selected_type) {
            case GradientEditState::MarkerType::Alpha: {
                gradient.alpha_markers[state.selected_index].location =
                    location;
                break;
            }
            case GradientEditState::MarkerType::Color: {
                gradient.color_markers[state.selected_index].location =
                    location;
                break;
            }
        }

        ImGui::TableNextColumn();
        if (ImGui::Button("Delete")) {
            switch (state.selected_type) {
                case GradientEditState::MarkerType::Alpha: {
                    gradient.remove_alpha_marker(state.selected_index);
                    break;
                }
                case GradientEditState::MarkerType::Color: {
                    gradient.remove_color_marker(state.selected_index);
                    break;
                }
            }
            state.selected_type = GradientEditState::MarkerType::None;
            state.selected_index = -1;
            state.dragging = false;
        }

        if (locked) {
            ImGui::EndDisabled();
        }

        if (disable) {
            ImGui::EndDisabled();
        }
    }
    ImGui::EndTable();

    ImGui::PopID();

    return changed;
}

template <typename T>
T sample_markers(std::vector<Gradient::Marker<T>> &markers, float location)
{
    location = std::clamp(location, 0.0f, 1.0f);
    auto it = std::lower_bound(markers.begin(), markers.end(), location);
    if (it == markers.begin()) {
        return it->value;
    }
    if (it == markers.end()) {
        return markers.back().value;
    }
    float curr = it->location, prev = (it - 1)->location;
    float t = (location - prev) / (curr - prev);
    return lerp((it - 1)->value, it->value, t);
}

template <typename T>
std::pair<bool, size_t> add_marker(
    std::vector<Gradient::Marker<T>> &markers,
    float location,
    const T &value)
{
    location = std::clamp(location, 0.0f, 1.0f);
    auto it = std::lower_bound(markers.begin(), markers.end(), location);
    if (it == markers.begin()) {
        it++;
    }
    if (it == markers.end()) {
        it = markers.end() - 1;
    }
    size_t index = std::distance(markers.begin(), it);
    markers.emplace(it, location, value);
    return {true, index};
}

template <typename T>
bool remove_marker(std::vector<T> &markers, size_t index)
{
    auto it = markers.begin() + index;
    if (it == markers.begin() || it == markers.end() - 1) {
        return false;
    }
    markers.erase(it);
    return true;
}

template <typename T, typename F>
std::pair<bool, bool> update_markers(
    const char *str_id,
    std::vector<T> &markers,
    GradientEditState &state,
    GradientEditState::MarkerType type,
    const ImVec2 &min,
    const ImVec2 &max,
    float marker_radius,
    F &&get_color)
{
    ImVec2 origin = ImGui::GetCursorScreenPos();

    bool changed = false;
    bool hovered = false;
    int new_selected_index = -1;

    // Update last marker first
    update_marker(
        str_id, markers, markers.size() - 1, state, type, min, max,
        marker_radius, get_color, changed, hovered, new_selected_index);

    // Update non-selected markers
    for (size_t i = 0; i < markers.size() - 1; i++) {
        if (i == state.selected_index) {
            continue;
        }
        update_marker(
            str_id, markers, i, state, type, min, max, marker_radius, get_color,
            changed, hovered, new_selected_index);
    }

    // Update selected marker
    bool selected_hovered = false;
    if (type == state.selected_type && state.selected_index != -1) {
        update_marker(
            str_id, markers, state.selected_index, state, type, min, max,
            marker_radius, get_color, changed, selected_hovered,
            new_selected_index);
        hovered |= selected_hovered;
    }

    // Set new selected
    if (new_selected_index != -1) {
        state.selected_index = new_selected_index;
        state.selected_type = type;
    }

    // Begin drag
    if (selected_hovered && ImGui::IsMouseDown(ImGuiMouseButton_Left) &&
        !ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
        state.dragging = state.selected_index > 0 &&
                         state.selected_index < markers.size() - 1;
    }

    // End drag
    if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        state.dragging = false;
    }

    // Ensure markers remain in sorted order
    if (state.selected_type == type && state.selected_index >= 0) {
        T marker = markers[state.selected_index];
        // Shuffle down
        while (state.selected_index > 1 &&
               marker < markers[state.selected_index - 1]) {
            markers[state.selected_index] = markers[state.selected_index - 1];
            state.selected_index--;
        }
        // Shuffle up
        while (state.selected_index < markers.size() - 2 &&
               marker > markers[state.selected_index + 1]) {
            markers[state.selected_index] = markers[state.selected_index + 1];
            state.selected_index++;
        }
        markers[state.selected_index] = marker;
    }

    ImGui::SetCursorScreenPos(origin);

    return {changed, hovered};
}

template <typename T, typename F>
void update_marker(
    const char *str_id,
    std::vector<T> &markers,
    int index,
    GradientEditState &state,
    GradientEditState::MarkerType type,
    const ImVec2 &min,
    const ImVec2 &max,
    float marker_radius,
    F &&get_color,
    bool &changed,
    bool &hovered,
    int &selected_index)
{
    T &marker = markers[index];

    float marker_height = max.y - min.y;
    bool selected =
        type == state.selected_type && index == state.selected_index;
    float x = std::lerp(min.x, max.x, marker.location);

    // Draw marker
    if (type == GradientEditState::MarkerType::Color) {
        draw_marker(
            ImVec2(x, min.y), ImVec2(x, max.y - marker_radius), marker_radius,
            get_color(marker), selected);
    } else {
        draw_marker(
            ImVec2(x, max.y), ImVec2(x, min.y + marker_radius), marker_radius,
            get_color(marker), selected);
    }

    // Draw invisible bounds
    ImGui::SetCursorScreenPos(ImVec2(x - marker_radius, min.y));
    ImGui::InvisibleButton(
        std::format("{}-{}", str_id, index).c_str(),
        ImVec2(marker_radius * 2.0f, marker_height));

    bool marker_hovered =
        ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);

    hovered |= marker_hovered;

    // Check if should select
    if (state.dragging == false && marker_hovered &&
        ImGui::IsMouseDown(ImGuiMouseButton_Left) &&
        !ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
        selected_index = index;
    }

    // Update location if dragging
    if (state.dragging && state.selected_index == index &&
        state.selected_type == type &&
        ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
        float delta = ImGui::GetIO().MouseDelta.x / (max.x - min.x);
        marker.location = std::clamp(marker.location + delta, 0.0f, 1.0f);

        changed |= delta > 0.0f;
    }
}

void draw_marker(
    const ImVec2 &start,
    const ImVec2 &end,
    float radius,
    const ImU32 &color,
    bool selected)
{
    ImDrawList *draw_list = ImGui::GetWindowDrawList();

    ImGuiStyle &style = ImGui::GetStyle();
    ImVec4 border_color = selected ? style.Colors[ImGuiCol_FrameBgHovered]
                                   : style.Colors[ImGuiCol_Border];
    ImU32 border_color_u32 = ImGui::ColorConvertFloat4ToU32(border_color);

    draw_list->AddLine(start, end, border_color_u32, 1.0f);
    draw_list->AddCircleFilled(end, radius, border_color_u32);
    draw_list->AddCircleFilled(end, radius - 1.0f, color);
}
