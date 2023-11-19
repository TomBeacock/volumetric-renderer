#include "main_window.h"

#include "application.h"
#include "data/file_parser.h"
#include "data/importer.h"
#include "imgui_context.h"
#include "rendering/offscreen_pass.h"
#include "rendering/vulkan_context.h"
#include "ui/ui_context.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <nfd.h>

namespace ImGui
{
void ImageRounded(
    ImTextureID user_texture_id,
    const ImVec2 &size,
    const ImVec2 &uv0 = ImVec2(0, 0),
    const ImVec2 &uv1 = ImVec2(1, 1),
    const ImVec4 &tint_col = ImVec4(1, 1, 1, 1),
    const ImVec4 &border_col = ImVec4(0, 0, 0, 0),
    float rounding = 0.0f)
{
    ImGuiWindow *window = GetCurrentWindow();
    if (window->SkipItems)
        return;

    ImRect bb(
        window->DC.CursorPos,
        ImVec2(
            window->DC.CursorPos.x + size.x, window->DC.CursorPos.y + size.y));
    if (border_col.w > 0.0f) {
        bb.Max = ImVec2(bb.Max.x + 2, bb.Max.y + 2);
    }
    ItemSize(bb);
    if (!ItemAdd(bb, 0))
        return;

    if (border_col.w > 0.0f) {
        window->DrawList->AddRect(
            bb.Min, bb.Max, GetColorU32(border_col), 0.0f);
        window->DrawList->AddImageRounded(
            user_texture_id, ImVec2(bb.Min.x + 1, bb.Min.y + 1),
            ImVec2(bb.Max.x - 1, bb.Max.y - 1), uv0, uv1, GetColorU32(tint_col),
            rounding);
    } else {
        window->DrawList->AddImageRounded(
            user_texture_id, bb.Min, bb.Max, uv0, uv1, GetColorU32(tint_col),
            rounding);
    }
}
}  // namespace ImGui

void Vol::UI::MainWindow::update()
{
    // Update content
    update_main_menu_bar();
    update_status_bar();
    update_main_window();
}

void Vol::UI::MainWindow::update_main_menu_bar()
{
    bool should_import = false;

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 6.0f));

    // Main menu bar
    if (ImGui::BeginMainMenuBar()) {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4.0f, 4.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 8.0f));
        if (ImGui::BeginMenu("File")) {
            // Open file menu item
            if (ImGui::MenuItem("Import...")) {
                should_import = true;
            }
            set_status_text_on_hover("Import a volumetric dataset from a file");

            ImGui::Separator();

            // Exit application menu item
            if (ImGui::MenuItem("Exit", "Alt+F4")) {
            }
            set_status_text_on_hover("Exit the application");
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Edit")) {
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View")) {
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Help")) {
            if (ImGui::MenuItem("About")) {
            }
            ImGui::EndMenu();
        }
        ImGui::PopStyleVar(2);
    }
    ImGui::EndMainMenuBar();

    ImGui::PopStyleVar();

    if (should_import) {
        if (auto filepath = open_file_dialog()) {
            Application::main().get_importer().import(*filepath);
        }
    }
}

void Vol::UI::MainWindow::update_status_bar()
{
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 6.0f));

    if (ImGui::BeginViewportSideBar(
            "statusbar", nullptr, ImGuiDir_Down, ImGui::GetFrameHeight(),
            ImGuiWindowFlags_NoDecoration)) {
        ImGui::AlignTextToFramePadding();
        ImGui::Text(status_text.c_str());
    }
    ImGui::End();

    ImGui::PopStyleVar();

    status_text = "";
}

void Vol::UI::MainWindow::update_main_window()
{
    // Create main window
    const ImGuiViewport *viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGuiWindowFlags window_flags =
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove;

    // Begin window
    if (ImGui::Begin("Main", nullptr, window_flags)) {
        // Create window content
        update_viewport();
        ImGui::SameLine();
        update_controls();
    }

    // End window
    ImGui::End();
}

void Vol::UI::MainWindow::update_viewport()
{
    // Begin child
    const ImGuiViewport *viewport = ImGui::GetMainViewport();
    float viewport_width = viewport->WorkSize.x - 400.0f;

    if (ImGui::BeginChild("viewport", ImVec2(viewport_width, 0.0f))) {
        ImVec2 scene_window_size = ImGui::GetWindowSize();
        uint32_t width = static_cast<uint32_t>(scene_window_size.x);
        uint32_t height = static_cast<uint32_t>(scene_window_size.y);

        if (width != current_scene_window_size.x ||
            height != current_scene_window_size.y) {
            current_scene_window_size = {width, height};
            Application::main().get_imgui_context().recreate_viewport_texture(
                width, height);
        }
        ImTextureID texture = static_cast<ImTextureID>(
            Application::main().get_imgui_context().get_descriptor());

        ImGuiStyle &style = ImGui::GetStyle();
        ImGui::ImageRounded(
            texture, scene_window_size, ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f),
            ImVec4(1.0f, 1.0f, 1.0f, 1.0f), ImVec4(0.0f, 0.0f, 0.0f, 0.0f),
            style.ChildRounding);
    }

    // End child
    ImGui::EndChild();
}

void Vol::UI::MainWindow::update_controls()
{
    // Push child style
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 8.0f));

    // Begin child
    if (ImGui::BeginChild(
            "controls", ImVec2(0.0f, 0.0f),
            ImGuiChildFlags_AlwaysUseWindowPadding)) {
        // Create child content
        ImFont *heading_font = ImGui::GetIO().Fonts->Fonts[1];
        ImGui::PushFont(heading_font);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
        ImGui::Text("Display");
        ImGui::PopStyleColor();
        ImGui::PopFont();

        ImGui::Dummy(ImVec2(0.0f, 4.0f));

        static float brightness = 0.0f, contrast = 0.0f;
        if (ImGui::BeginTable("display_controls", 3)) {
            ImGui::TableSetupColumn(
                "label_col", ImGuiTableColumnFlags_WidthFixed, 100.0f);
            ImGui::TableSetupColumn(
                "slider_col", ImGuiTableColumnFlags_WidthFixed, 200.0f);
            ImGui::TableSetupColumn(
                "field_col", ImGuiTableColumnFlags_WidthFixed, 100.0f);

            slider(
                "Brightness", &brightness, 0.0f, 100.0f,
                "Adjust visualization brightness");
            slider(
                "Contrast", &contrast, 0.0f, 100.0f,
                "Adjust visualization contrast");
        }
        ImGui::EndTable();

        ImGui::Separator();
    }

    // End child
    ImGui::EndChild();

    // Pop child style
    ImGui::PopStyleVar();
}

void Vol::UI::MainWindow::set_status_text_on_hover(const std::string &text)
{
    if (ImGui::IsItemHovered()) {
        status_text = text;
    }
}

void Vol::UI::MainWindow::slider(
    const std::string &label,
    float *v,
    float v_min,
    float v_max,
    const std::string &hint)
{
    ImGui::TableNextRow();

    ImGui::TableSetColumnIndex(0);
    ImGui::PushItemWidth(-FLT_MIN);
    ImGui::AlignTextToFramePadding();
    ImGui::Text(label.c_str());

    ImGui::TableSetColumnIndex(1);
    ImGui::PushItemWidth(-FLT_MIN);
    std::string slider_label = "##" + label + "_slider";
    ImGui::SliderFloat(slider_label.c_str(), v, v_min, v_max, "");
    set_status_text_on_hover(hint);

    ImGui::TableSetColumnIndex(2);
    ImGui::PushItemWidth(-FLT_MIN);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1);
    std::string input_label = "##" + label + "_input";
    ImGui::InputFloat(input_label.c_str(), v, 0.0f, 0.0f, "%.1f");
    set_status_text_on_hover(hint);
    ImGui::PopStyleVar();
}

std::optional<std::filesystem::path> Vol::UI::MainWindow::open_file_dialog()
    const
{
    nfdchar_t *out_path;
    nfdfilteritem_t filter_items[1] = {{"Nearly Raw Raster Data", "nffd,ndhr"}};
    if (NFD_OpenDialog(&out_path, filter_items, 1, nullptr) == NFD_OKAY) {
        std::filesystem::path path(out_path);
        NFD_FreePath(out_path);
        return path;
    }
    return std::nullopt;
}
