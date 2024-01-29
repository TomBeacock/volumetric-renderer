#include "ui_context.h"

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_vulkan.h>
#include <imgui_internal.h>
#include <nfd.h>

Vol::UI::UIContext::UIContext()
{
    // Set style
    ImGuiStyle &style = ImGui::GetStyle();

    // Style variables
    style.WindowBorderSize = 0.0f;
    style.WindowPadding = ImVec2(4.0f, 0.0f);
    style.ChildRounding = 4.0f;
    style.PopupRounding = 2.0f;
    style.FrameRounding = 4.0f;
    style.GrabRounding = 10.0f;

    // Colors
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
    style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
    style.Colors[ImGuiCol_FrameBg] = ImVec4(0.102f, 0.106f, 0.110f, 1.0f);
    style.Colors[ImGuiCol_ChildBg] = ImVec4(0.169f, 0.173f, 0.184f, 1.0f);
    style.Colors[ImGuiCol_PopupBg] = ImVec4(0.169f, 0.173f, 0.184f, 1.0f);

    style.Colors[ImGuiCol_Border] = ImVec4(0.212f, 0.220f, 0.243f, 1.0f);

    style.Colors[ImGuiCol_Text] = ImVec4(0.765f, 0.765f, 0.765f, 1.0f);
    style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.120f, 0.120f, 0.130f, 1.0f);
    style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.690f, 0.690f, 0.690f, 1.0f);
    style.Colors[ImGuiCol_Separator] = ImVec4(0.110f, 0.110f, 0.110f, 1.0f);
}

void Vol::UI::UIContext::update()
{
    // Begin ImGui frame
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    // Update content
    main_window.update();
    error_popup.update();

    // Display demo window
    // ImGui::ShowDemoWindow((bool *)0);
}

void Vol::UI::UIContext::show_error(
    const std::string &title,
    const std::string &message)
{
    error_popup.show(title, message);
}
