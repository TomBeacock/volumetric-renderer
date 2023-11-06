#include "ui_context.h"

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_vulkan.h>
#include <imgui_internal.h>
#include <nfd.h>

Vol::UI::UIContext::UIContext() {
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

void Vol::UI::UIContext::update() {
    // Begin ImGui frame
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    // Update content
    main_window.update();

    // Display demo window
    ImGui::ShowDemoWindow((bool *)0);
}
