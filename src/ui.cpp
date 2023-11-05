#include "ui.h"

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_vulkan.h>
#include <imgui_internal.h>

Vol::UI::UI() {
    // Set style
    ImGuiStyle &style = ImGui::GetStyle();

    // Style variables
    style.WindowBorderSize = 0.0f;
    style.WindowPadding = ImVec2(4.0f, 0.0f);
    style.ChildRounding = 4.0f;
    style.PopupRounding = 2.0f;
    style.FrameRounding = 2.0f;
    style.GrabRounding = 10.0f;

    // Colors
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
    style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
    style.Colors[ImGuiCol_FrameBg] = ImVec4(0.00802f, 0.00857f, 0.00913f, 1.0f);
    style.Colors[ImGuiCol_ChildBg] = ImVec4(0.02416f, 0.02519f, 0.02843f, 1.0f);
    style.Colors[ImGuiCol_PopupBg] = ImVec4(0.02416f, 0.02519f, 0.02843f, 1.0f);

    style.Colors[ImGuiCol_Border] = ImVec4(0.03689f, 0.03955f, 0.04817f, 1.0f);

    style.Colors[ImGuiCol_Text] = ImVec4(0.54572f, 0.54572f, 0.54572f, 1.0f);
    style.Colors[ImGuiCol_HeaderHovered] =
        ImVec4(0.0137f, 0.0137f, 0.01444f, 1.0f);
    style.Colors[ImGuiCol_SliderGrab] =
        ImVec4(0.43415f, 0.43415f, 0.42869f, 1.0f);
    style.Colors[ImGuiCol_Separator] =
        ImVec4(0.00802f, 0.00857f, 0.00913f, 1.0f);
}

void Vol::UI::update() {
    // Begin ImGui frame
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    // Update content
    update_main_menu_bar();
    update_status_bar();
    update_main_window();

    // Display demo window
    ImGui::ShowDemoWindow((bool *)0);
}

void Vol::UI::update_main_menu_bar() {
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 6.0f));

    // Main menu bar
    if (ImGui::BeginMainMenuBar()) {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4.0f, 4.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 8.0f));
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Open", "Ctrl+O")) {
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Exit", "Alt+F4")) {
            }
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
}

void Vol::UI::update_status_bar() {
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 6.0f));

    if (ImGui::BeginViewportSideBar(
            "statusbar", nullptr, ImGuiDir_Down, ImGui::GetFrameHeight(),
            ImGuiWindowFlags_NoDecoration
        )) {
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Status bar");
    }
    ImGui::End();

    ImGui::PopStyleVar();
}

void Vol::UI::update_main_window() {
    // Create main window
    const ImGuiViewport *viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGuiWindowFlags window_flags =
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove;

    // Begin window
    if (ImGui::Begin("Main", (bool *)0, window_flags)) {
        // Create window content
        update_viewport();
        ImGui::SameLine();
        update_controls();
    }

    // End window
    ImGui::End();
}

void Vol::UI::update_viewport() {
    // Push child style
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.11f, 0.11f, 0.11f, 1.0f));

    // Begin child
    const ImGuiViewport *viewport = ImGui::GetMainViewport();
    float viewport_width = viewport->WorkSize.x - 400.0f;
    if (ImGui::BeginChild("viewport", ImVec2(viewport_width, 0.0f))) {
    }

    // End child
    ImGui::EndChild();

    // Pop child style
    ImGui::PopStyleColor();
}

void Vol::UI::update_controls() {
    // Push child style
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 8.0f));

    // Begin child
    if (ImGui::BeginChild(
            "controls", ImVec2(0.0f, 0.0f),
            ImGuiChildFlags_AlwaysUseWindowPadding
        )) {
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
                "label_col", ImGuiTableColumnFlags_WidthFixed, 100.0f
            );
            ImGui::TableSetupColumn(
                "slider_col", ImGuiTableColumnFlags_WidthFixed, 200.0f
            );
            ImGui::TableSetupColumn(
                "field_col", ImGuiTableColumnFlags_WidthFixed, 100.0f
            );

            slider("Brightness", &brightness, 0.0f, 100.0f);
            slider("Contrast", &contrast, 0.0f, 100.0f);
        }
        ImGui::EndTable();

        ImGui::Separator();
    }

    // End child
    ImGui::EndChild();

    // Pop child style
    ImGui::PopStyleVar();
}

void Vol::UI::slider(
    const std::string &label,
    float *v,
    float v_min,
    float v_max
) {
    ImGui::TableNextRow();

    ImGui::TableSetColumnIndex(0);
    ImGui::PushItemWidth(-FLT_MIN);
    ImGui::AlignTextToFramePadding();
    ImGui::Text(label.c_str());

    ImGui::TableSetColumnIndex(1);
    ImGui::PushItemWidth(-FLT_MIN);
    std::string slider_label = "##" + label + "_slider";
    ImGui::SliderFloat(slider_label.c_str(), v, v_min, v_max, "");

    ImGui::TableSetColumnIndex(2);
    ImGui::PushItemWidth(-FLT_MIN);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1);
    std::string input_label = "##" + label + "_input";
    ImGui::InputFloat(input_label.c_str(), v, 0.0f, 0.0f, "%.1f");
    ImGui::PopStyleVar();
}
