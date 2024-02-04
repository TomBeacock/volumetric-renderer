#include "slider.h"

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif

#include "slider.h"

#include <imgui.h>
#include <imgui_internal.h>

float round_float_with_format(
    const char *format,
    ImGuiDataType data_type,
    float v)
{
    return ImGui::RoundScalarWithFormatT<float>(format, data_type, v);
}

float scale_ratio_from_value(
    ImGuiDataType data_type,
    float v,
    float v_min,
    float v_max,
    float power,
    float linear_zero_pos)
{
    return ImGui::ScaleRatioFromValueT<float, float, float>(
        data_type, v, v_min, v_max, false, power, linear_zero_pos);
}

bool slider_scalar(
    const char *label,
    ImGuiDataType data_type,
    void *data,
    void *min,
    void *max,
    const char *format,
    ImGuiSliderFlags flags);

bool range_slider_scalar(
    const char *label,
    ImGuiDataType data_type,
    float *value_min,
    float *value_max,
    float min,
    float max,
    const char *format,
    ImGuiSliderFlags flags);

bool range_slider_behavior(
    const ImRect &bb,
    ImGuiID id,
    float *value_min,
    float *value_max,
    const float min,
    const float max,
    float power,
    int decimal_precision,
    ImGuiSliderFlags flags,
    ImRect *out_grab_bb1,
    ImRect *out_grab_bb2);

bool Vol::UI::Components::slider_float(
    const char *label,
    float *value,
    float min,
    float max,
    const char *format,
    ImGuiSliderFlags flags)
{
    return slider_scalar(
        label, ImGuiDataType_Float, value, &min, &max, format, flags);
}

bool Vol::UI::Components::range_slider_float(
    const char *label,
    float *value_min,
    float *value_max,
    float min,
    float max,
    const char *format,
    ImGuiSliderFlags flags)
{
    return range_slider_scalar(
        label, ImGuiDataType_Float, value_min, value_max, min, max, format,
        flags);
}

bool slider_scalar(
    const char *label,
    ImGuiDataType data_type,
    void *data,
    void *min,
    void *max,
    const char *format,
    ImGuiSliderFlags flags)
{
    using namespace ImGui;

    ImGuiWindow *window = GetCurrentWindow();
    if (window->SkipItems) {
        return false;
    }

    ImGuiContext &g = *GImGui;
    const ImGuiStyle &style = g.Style;
    const ImGuiID id = window->GetID(label);
    const float w = CalcItemWidth();

    const ImVec2 label_size = CalcTextSize(label, NULL, true);
    const ImRect frame_bb(
        window->DC.CursorPos,
        window->DC.CursorPos +
            ImVec2(w, label_size.y + style.FramePadding.y * 2.0f));
    const ImRect total_bb(
        frame_bb.Min,
        frame_bb.Max + ImVec2(
                           label_size.x > 0.0f
                               ? style.ItemInnerSpacing.x + label_size.x
                               : 0.0f,
                           0.0f));

    const bool temp_input_allowed = (flags & ImGuiSliderFlags_NoInput) == 0;
    ItemSize(total_bb, style.FramePadding.y);
    if (!ItemAdd(
            total_bb, id, &frame_bb,
            temp_input_allowed ? ImGuiItemFlags_Inputable : 0)) {
        return false;
    }

    // Default format string when passing NULL
    if (format == nullptr) {
        format = DataTypeGetInfo(data_type)->PrintFmt;
    }

    const bool hovered = ItemHoverable(frame_bb, id, g.LastItemData.InFlags);
    bool temp_input_is_active = temp_input_allowed && TempInputIsActive(id);
    if (!temp_input_is_active) {
        // Tabbing or CTRL-clicking on Slider turns it into an input box
        const bool input_requested_by_tabbing =
            temp_input_allowed && (g.LastItemData.StatusFlags &
                                   ImGuiItemStatusFlags_FocusedByTabbing) != 0;
        const bool clicked = hovered && IsMouseClicked(0, id);
        const bool make_active =
            (input_requested_by_tabbing || clicked || g.NavActivateId == id);
        if (make_active && clicked) {
            SetKeyOwner(ImGuiKey_MouseLeft, id);
        }
        if (make_active && temp_input_allowed) {
            if (input_requested_by_tabbing || (clicked && g.IO.KeyCtrl) ||
                (g.NavActivateId == id &&
                 (g.NavActivateFlags & ImGuiActivateFlags_PreferInput))) {
                temp_input_is_active = true;
            }
        }

        if (make_active && !temp_input_is_active) {
            SetActiveID(id, window);
            SetFocusID(id, window);
            FocusWindow(window);
            g.ActiveIdUsingNavDirMask |=
                (1 << ImGuiDir_Left) | (1 << ImGuiDir_Right);
        }
    }

    if (temp_input_is_active) {
        // Only clamp CTRL+Click input when ImGuiSliderFlags_AlwaysClamp is set
        const bool is_clamp_input = (flags & ImGuiSliderFlags_AlwaysClamp) != 0;
        return TempInputScalar(
            frame_bb, id, label, data_type, data, format,
            is_clamp_input ? min : NULL, is_clamp_input ? max : NULL);
    }

    // Draw frame
    float frame_middle = frame_bb.GetCenter().y;
    ImRect track_bb(
        frame_bb.Min.x, frame_middle - 2.0f, frame_bb.Max.x,
        frame_middle + 2.0f);
    RenderFrame(
        track_bb.Min, track_bb.Max, GetColorU32(ImGuiCol_FrameBg), false, 2.0f);

    // Slider behavior
    ImRect grab_bb;
    const bool value_changed = SliderBehavior(
        frame_bb, id, data_type, data, min, max, format, flags, &grab_bb);
    if (value_changed) {
        MarkItemEdited(id);
    }

    // Render fill
    ImVec2 grab_center = grab_bb.GetCenter();
    RenderFrame(
        track_bb.Min, ImVec2(grab_center.x, track_bb.Max.y),
        GetColorU32(ImGuiCol_SliderGrabActive), false, 0.0f);

    // Render grab
    if (grab_bb.Max.x > grab_bb.Min.x) {
        window->DrawList->AddCircleFilled(
            grab_center, grab_bb.GetHeight() / 2.5f,
            GetColorU32(
                g.ActiveId == id ? ImGuiCol_SliderGrabActive
                                 : ImGuiCol_SliderGrab),
            24);
    }

    // Display value using user-provided display format so user can add
    // prefix/suffix/decorations to the value.
    char value_buf[64];
    const char *value_buf_end =
        value_buf +
        DataTypeFormatString(
            value_buf, IM_ARRAYSIZE(value_buf), data_type, data, format);
    if (g.LogEnabled) {
        LogSetNextTextDecoration("{", "}");
    }
    RenderTextClipped(
        frame_bb.Min, frame_bb.Max, value_buf, value_buf_end, NULL,
        ImVec2(0.5f, 0.5f));

    if (label_size.x > 0.0f) {
        RenderText(
            ImVec2(
                frame_bb.Max.x + style.ItemInnerSpacing.x,
                frame_bb.Min.y + style.FramePadding.y),
            label);
    }

    IMGUI_TEST_ENGINE_ITEM_INFO(
        id, label,
        g.LastItemData.StatusFlags |
            (temp_input_allowed ? ImGuiItemStatusFlags_Inputable : 0));
    return value_changed;
}

bool range_slider_scalar(
    const char *label,
    ImGuiDataType data_type,
    float *value_min,
    float *value_max,
    float min,
    float max,
    const char *format,
    ImGuiSliderFlags flags)
{
    using namespace ImGui;

    ImGuiWindow *window = GetCurrentWindow();
    if (window->SkipItems) {
        return false;
    }

    ImGuiContext &g = *GImGui;
    const ImGuiStyle &style = g.Style;
    const ImGuiID id = window->GetID(label);
    const float w = CalcItemWidth();

    const ImVec2 label_size = CalcTextSize(label, NULL, true);
    const ImRect frame_bb(
        window->DC.CursorPos,
        window->DC.CursorPos +
            ImVec2(w, label_size.y + style.FramePadding.y * 2.0f));
    const ImRect total_bb(
        frame_bb.Min,
        frame_bb.Max + ImVec2(
                           label_size.x > 0.0f
                               ? style.ItemInnerSpacing.x + label_size.x
                               : 0.0f,
                           0.0f));

    const bool temp_input_allowed = (flags & ImGuiSliderFlags_NoInput) == 0;
    ItemSize(total_bb, style.FramePadding.y);
    if (!ItemAdd(
            total_bb, id, &frame_bb,
            temp_input_allowed ? ImGuiItemFlags_Inputable : 0)) {
        return false;
    }

    // Default format string when passing NULL
    if (format == nullptr) {
        format = DataTypeGetInfo(data_type)->PrintFmt;
    }

    const bool hovered = ItemHoverable(frame_bb, id, g.LastItemData.InFlags);
    bool temp_input_is_active = temp_input_allowed && TempInputIsActive(id);
    if (!temp_input_is_active) {
        // Tabbing or CTRL-clicking on Slider turns it into an input box
        const bool input_requested_by_tabbing =
            temp_input_allowed && (g.LastItemData.StatusFlags &
                                   ImGuiItemStatusFlags_FocusedByTabbing) != 0;
        const bool clicked = hovered && IsMouseClicked(0, id);
        const bool make_active =
            (input_requested_by_tabbing || clicked || g.NavActivateId == id);
        if (make_active && clicked) {
            SetKeyOwner(ImGuiKey_MouseLeft, id);
        }
        if (make_active && temp_input_allowed) {
            if (input_requested_by_tabbing || (clicked && g.IO.KeyCtrl) ||
                (g.NavActivateId == id &&
                 (g.NavActivateFlags & ImGuiActivateFlags_PreferInput))) {
                temp_input_is_active = true;
            }
        }

        if (make_active && !temp_input_is_active) {
            SetActiveID(id, window);
            SetFocusID(id, window);
            FocusWindow(window);
            g.ActiveIdUsingNavDirMask |=
                (1 << ImGuiDir_Left) | (1 << ImGuiDir_Right);
        }
    }

    if (temp_input_is_active) {
        // Only clamp CTRL+Click input when ImGuiSliderFlags_AlwaysClamp is set
        const bool is_clamp_input = (flags & ImGuiSliderFlags_AlwaysClamp) != 0;
        return TempInputScalar(
            frame_bb, id, label, data_type, value_min, format,
            is_clamp_input ? &min : NULL, is_clamp_input ? &max : NULL);
    }

    // Draw frame
    float frame_middle = frame_bb.GetCenter().y;
    ImRect track_bb(
        frame_bb.Min.x, frame_middle - 2.0f, frame_bb.Max.x,
        frame_middle + 2.0f);
    RenderFrame(
        track_bb.Min, track_bb.Max, GetColorU32(ImGuiCol_FrameBg), false, 2.0f);

    // Slider behavior
    int decimal_precision = ImParseFormatPrecision(format, 3);
    ImRect grab_bb1, grab_bb2;
    const bool value_changed = range_slider_behavior(
        frame_bb, id, value_min, value_max, min, max, 1.0f, decimal_precision,
        flags, &grab_bb1, &grab_bb2);
    if (value_changed) {
        MarkItemEdited(id);
    }

    // Render fill
    RenderFrame(
        ImVec2(grab_bb1.GetCenter().x, track_bb.Min.y),
        ImVec2(grab_bb2.GetCenter().x, track_bb.Max.y),
        GetColorU32(ImGuiCol_SliderGrabActive), false, 0.0f);

    // Render grab min
    if (grab_bb1.Max.x > grab_bb1.Min.x) {
        window->DrawList->AddCircleFilled(
            grab_bb1.GetCenter(), grab_bb1.GetHeight() / 2.5f,
            GetColorU32(
                g.ActiveId == id ? ImGuiCol_SliderGrabActive
                                 : ImGuiCol_SliderGrab),
            24);
    }

    // Render grab max
    if (grab_bb2.Max.x > grab_bb2.Min.x) {
        window->DrawList->AddCircleFilled(
            grab_bb2.GetCenter(), grab_bb2.GetHeight() / 2.5f,
            GetColorU32(
                g.ActiveId == id ? ImGuiCol_SliderGrabActive
                                 : ImGuiCol_SliderGrab),
            24);
    }

    // Display value using user-provided display format so user can add
    // prefix/suffix/decorations to the value.
    char value_buf[64];
    const char *value_buf_end =
        value_buf +
        DataTypeFormatString(
            value_buf, IM_ARRAYSIZE(value_buf), data_type, value_min, format);
    if (g.LogEnabled) {
        LogSetNextTextDecoration("{", "}");
    }
    RenderTextClipped(
        frame_bb.Min, frame_bb.Max, value_buf, value_buf_end, NULL,
        ImVec2(0.5f, 0.5f));

    if (label_size.x > 0.0f) {
        RenderText(
            ImVec2(
                frame_bb.Max.x + style.ItemInnerSpacing.x,
                frame_bb.Min.y + style.FramePadding.y),
            label);
    }

    IMGUI_TEST_ENGINE_ITEM_INFO(
        id, label,
        g.LastItemData.StatusFlags |
            (temp_input_allowed ? ImGuiItemStatusFlags_Inputable : 0));
    return value_changed;
}

bool range_slider_behavior(
    const ImRect &frame_bb,
    ImGuiID id,
    float *value_min,
    float *value_max,
    const float min,
    const float max,
    float power,
    int decimal_precision,
    ImGuiSliderFlags flags,
    ImRect *out_grab_bb1,
    ImRect *out_grab_bb2)
{
    using namespace ImGui;

    ImGuiContext &g = *GImGui;
    ImGuiWindow *window = GetCurrentWindow();
    const ImGuiStyle &style = g.Style;

    // Draw frame
    float frame_middle = frame_bb.GetCenter().y;
    ImRect track_bb(
        frame_bb.Min.x, frame_middle - 2.0f, frame_bb.Max.x,
        frame_middle + 2.0f);
    RenderFrame(
        track_bb.Min, track_bb.Max, GetColorU32(ImGuiCol_FrameBg), false, 2.0f);

    const bool is_non_linear =
        (power < 1.0f - 0.00001f) || (power > 1.0f + 0.00001f);
    const bool is_horizontal = (flags & ImGuiSliderFlags_Vertical) == 0;

    const float grab_padding = 2.0f;
    const float slider_sz = is_horizontal
                                ? (frame_bb.GetWidth() - grab_padding * 2.0f)
                                : (frame_bb.GetHeight() - grab_padding * 2.0f);
    float grab_sz;
    if (decimal_precision > 0)
        grab_sz = ImMin(style.GrabMinSize, slider_sz);
    else
        grab_sz = ImMin(
            ImMax(
                1.0f *
                    (slider_sz / ((min < max ? max - min : min - max) + 1.0f)),
                style.GrabMinSize),
            slider_sz);  // Integer sliders, if possible have the grab size
                         // represent 1 unit
    const float slider_usable_sz = slider_sz - grab_sz;
    const float slider_usable_pos_min =
        (is_horizontal ? frame_bb.Min.x : frame_bb.Min.y) + grab_padding +
        grab_sz * 0.5f;
    const float slider_usable_pos_max =
        (is_horizontal ? frame_bb.Max.x : frame_bb.Max.y) - grab_padding -
        grab_sz * 0.5f;

    // For logarithmic sliders that cross over sign boundary we want the
    // exponential increase to be symmetric around 0.0f
    float linear_zero_pos = 0.0f;  // 0.0->1.0f
    if (min * max < 0.0f) {
        // Different sign
        const float linear_dist_min_to_0 =
            powf(fabsf(0.0f - min), 1.0f / power);
        const float linear_dist_max_to_0 =
            powf(fabsf(max - 0.0f), 1.0f / power);
        linear_zero_pos = linear_dist_min_to_0 /
                          (linear_dist_min_to_0 + linear_dist_max_to_0);
    } else {
        // Same sign
        linear_zero_pos = min < 0.0f ? 1.0f : 0.0f;
    }

    // Process clicking on the slider
    bool value_changed = false;
    if (g.ActiveId == id) {
        if (g.IO.MouseDown[0]) {
            const float mouse_abs_pos =
                is_horizontal ? g.IO.MousePos.x : g.IO.MousePos.y;
            float clicked_t =
                (slider_usable_sz > 0.0f)
                    ? ImClamp(
                          (mouse_abs_pos - slider_usable_pos_min) /
                              slider_usable_sz,
                          0.0f, 1.0f)
                    : 0.0f;
            if (!is_horizontal)
                clicked_t = 1.0f - clicked_t;

            float new_value;
            if (is_non_linear) {
                // Account for logarithmic scale on both sides of the zero
                if (clicked_t < linear_zero_pos) {
                    // Negative: rescale to the negative range before powering
                    float a = 1.0f - (clicked_t / linear_zero_pos);
                    a = powf(a, power);
                    new_value = ImLerp(ImMin(max, 0.0f), min, a);
                } else {
                    // Positive: rescale to the positive range before powering
                    float a;
                    if (fabsf(linear_zero_pos - 1.0f) > 1.e-6f)
                        a = (clicked_t - linear_zero_pos) /
                            (1.0f - linear_zero_pos);
                    else
                        a = clicked_t;
                    a = powf(a, power);
                    new_value = ImLerp(ImMax(min, 0.0f), max, a);
                }
            } else {
                // Linear slider
                new_value = ImLerp(min, max, clicked_t);
            }

            char fmt[64];
            snprintf(fmt, 64, "%%.%df", decimal_precision);

            // Round past decimal precision
            new_value =
                round_float_with_format(fmt, ImGuiDataType_Float, new_value);
            if (*value_min != new_value || *value_max != new_value) {
                if (fabsf(*value_min - new_value) <
                    fabsf(*value_max - new_value)) {
                    *value_min = new_value;
                } else {
                    *value_max = new_value;
                }
                value_changed = true;
            }
        } else {
            ClearActiveID();
        }
    }

    // Calculate slider grab min positioning
    float grab_t = scale_ratio_from_value(
        ImGuiDataType_Float, *value_min, min, max, power, linear_zero_pos);

    if (!is_horizontal) {
        grab_t = 1.0f - grab_t;
    }
    float grab_pos =
        ImLerp(slider_usable_pos_min, slider_usable_pos_max, grab_t);
    if (is_horizontal) {
        *out_grab_bb1 = ImRect(
            ImVec2(grab_pos - grab_sz * 0.5f, frame_bb.Min.y + grab_padding),
            ImVec2(grab_pos + grab_sz * 0.5f, frame_bb.Max.y - grab_padding));
    } else {
        *out_grab_bb1 = ImRect(
            ImVec2(frame_bb.Min.x + grab_padding, grab_pos - grab_sz * 0.5f),
            ImVec2(frame_bb.Max.x - grab_padding, grab_pos + grab_sz * 0.5f));
    }

    // Calculate slider grab max positioning
    grab_t = scale_ratio_from_value(
        ImGuiDataType_Float, *value_max, min, max, power, linear_zero_pos);

    if (!is_horizontal) {
        grab_t = 1.0f - grab_t;
    }
    grab_pos = ImLerp(slider_usable_pos_min, slider_usable_pos_max, grab_t);
    if (is_horizontal) {
        *out_grab_bb2 = ImRect(
            ImVec2(grab_pos - grab_sz * 0.5f, frame_bb.Min.y + grab_padding),
            ImVec2(grab_pos + grab_sz * 0.5f, frame_bb.Max.y - grab_padding));
    } else {
        *out_grab_bb2 = ImRect(
            ImVec2(frame_bb.Min.x + grab_padding, grab_pos - grab_sz * 0.5f),
            ImVec2(frame_bb.Max.x - grab_padding, grab_pos + grab_sz * 0.5f));
    }

    return value_changed;
}
