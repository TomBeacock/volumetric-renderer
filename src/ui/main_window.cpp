#include "main_window.h"

#include "application.h"
#include "data/file_parser.h"
#include "data/importer.h"
#include "imgui_context.h"
#include "rendering/offscreen_pass.h"
#include "rendering/vulkan_context.h"
#include "scene/scene.h"
#include "ui/components/gradient.h"
#include "ui/components/image_rounded.h"
#include "ui/components/slider.h"
#include "ui/ui_context.h"

#include <SDL3/SDL.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <nfd.h>

void Vol::UI::MainWindow::update()
{
    // Update content
    update_main_menu_bar();
    update_status_bar();
    update_main_window();
}

void Vol::UI::MainWindow::update_main_menu_bar()
{
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 6.0f));

    // Main menu bar
    if (ImGui::BeginMainMenuBar()) {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4.0f, 4.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 8.0f));
        if (ImGui::BeginMenu("File")) {
            // Open file menu item
            if (ImGui::BeginMenu("Import")) {
                if (ImGui::MenuItem("Nearly Raw Raster Data (.nrrd)")) {
                    Application::main().get_importer().import(
                        Vol::Data::FileFormat::Nrrd);
                }
                ImGui::EndMenu();
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
}

void Vol::UI::MainWindow::update_status_bar()
{
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 6.0f));

    if (ImGui::BeginViewportSideBar(
            "statusbar", nullptr, ImGuiDir_Down, ImGui::GetFrameHeight(),
            ImGuiWindowFlags_NoDecoration)) {
        ImGui::AlignTextToFramePadding();
        ImGui::Text(status_text.c_str());

        ImGui::SameLine();
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));

        std::string framerate_text = std::format("{:.0f} fps", framerate);

        ImVec2 text_size = ImGui::CalcTextSize(framerate_text.c_str());
        ImVec2 region = ImGui::GetContentRegionAvail();
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + region.x - text_size.x);

        ImGui::AlignTextToFramePadding();
        ImGui::Text(framerate_text.c_str());

        ImGui::PopStyleVar();
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
    Application &app = Application::main();
    ImGuiContext &context = app.get_imgui_context();

    // Begin child
    const ImGuiViewport *viewport = ImGui::GetMainViewport();
    float viewport_width = viewport->WorkSize.x - 400.0f;

    if (ImGui::BeginChild("viewport", ImVec2(viewport_width, 0.0f))) {
        ImVec2 scene_window_pos = ImGui::GetWindowPos();
        ImVec2 scene_window_size = ImGui::GetWindowSize();

        uint32_t width = static_cast<uint32_t>(scene_window_size.x);
        uint32_t height = static_cast<uint32_t>(scene_window_size.y);

        // Recreate texture if viewport size has changed
        if (width != current_scene_window_size.x ||
            height != current_scene_window_size.y) {
            current_scene_window_size = {width, height};
            context.recreate_viewport_texture(width, height);
        }

        // Draw scene as image
        ImTextureID texture =
            static_cast<ImTextureID>(context.get_descriptor());
        ImGuiStyle &style = ImGui::GetStyle();
        Components::image_rounded(
            texture, scene_window_size, ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f),
            ImVec4(1.0f, 1.0f, 1.0f, 1.0f), ImVec4(0.0f, 0.0f, 0.0f, 0.0f),
            style.ChildRounding);

        // Update viewport camera
        glm::vec2 min_bound(scene_window_pos.x, scene_window_pos.y);
        glm::vec2 max_bound =
            min_bound + glm::vec2(scene_window_size.x, scene_window_size.y);
        update_viewport_rotation(min_bound, max_bound);
        update_viewport_zoom();
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
        heading("Display");

        static float brightness = 0.0f, contrast = 0.0f;
        if (ImGui::BeginTable("display_controls", 2)) {
            ImGui::TableSetupColumn(
                "label", ImGuiTableColumnFlags_WidthFixed, 84.0f);
            ImGui::TableSetupColumn("field", ImGuiTableColumnFlags_WidthFixed);

            Components::slider(
                "Brightness", &brightness, 0.0f, 100.0f,
                "Adjust visualization brightness", status_text);
            Components::slider(
                "Contrast", &contrast, 0.0f, 100.0f,
                "Adjust visualization contrast", status_text);
        }
        ImGui::EndTable();

        ImGui::Dummy(ImVec2(0.0f, 8.0f));
        ImGui::Separator();
        ImGui::Dummy(ImVec2(0.0f, 8.0f));

        heading("Transfer function");

        static Components::Gradient gradient;
        static Components::GradientEditState gradient_state;
        if (Components::gradient_edit(
                "transfer_func", gradient, gradient_state, status_text)) {
            std::vector<uint32_t> gradient_data = gradient.discretize(256);
            Application::main()
                .get_vulkan_context()
                .get_offscreen_pass()
                ->transfer_function_changed(gradient_data);
        }
    }

    // End child
    ImGui::EndChild();

    // Pop child style
    ImGui::PopStyleVar();
}

void Vol::UI::MainWindow::update_viewport_rotation(
    const glm::vec2 &min_bound,
    const glm::vec2 &max_bound)
{
    static bool did_warp = false;

    // Ignore mouse motion after warp
    if (did_warp) {
        did_warp = false;
        ImGui::ResetMouseDragDelta();
        return;
    }

    Application &app = Application::main();
    Vol::Scene::Camera &camera = app.get_scene().get_camera();

    // If viewport being dragged on
    if (ImGui::IsWindowFocused() &&
        ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
        // Rotate camera by drag delta
        glm::vec2 drag_delta(
            ImGui::GetMouseDragDelta().x, ImGui::GetMouseDragDelta().y);
        camera.rotate(drag_delta);

        // Check if mouse cursor has left the viewport bounds
        glm::vec2 mouse_pos(ImGui::GetMousePos().x, ImGui::GetMousePos().y);

        bool out_of_bounds_x =
            mouse_pos.x < min_bound.x || mouse_pos.x > max_bound.x;
        bool out_of_bounds_y =
            mouse_pos.y < min_bound.y || mouse_pos.y > max_bound.y;

        // If left viewport bounds, warp cursor to the opposite side
        if (out_of_bounds_x || out_of_bounds_y) {
            if (out_of_bounds_x) {
                mouse_pos.x =
                    mouse_pos.x < min_bound.x ? max_bound.x : min_bound.x;
            }
            if (out_of_bounds_y) {
                mouse_pos.y =
                    mouse_pos.y < min_bound.y ? max_bound.y : min_bound.y;
            }

            SDL_WarpMouseInWindow(&app.get_window(), mouse_pos.x, mouse_pos.y);
            did_warp = true;
        }
        ImGui::ResetMouseDragDelta();
    }
}

void Vol::UI::MainWindow::update_viewport_zoom()
{
    if (ImGui::IsItemHovered()) {
        Vol::Scene::Camera &camera =
            Application::main().get_scene().get_camera();
        camera.zoom(ImGui::GetIO().MouseWheel);
    }
}

void Vol::UI::MainWindow::set_status_text_on_hover(const std::string &text)
{
    if (ImGui::IsItemHovered()) {
        status_text = text;
    }
}

void Vol::UI::MainWindow::heading(const std::string &text)
{
    ImFont *heading_font = ImGui::GetIO().Fonts->Fonts[1];
    ImGui::PushFont(heading_font);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
    ImGui::Text(text.c_str());
    ImGui::PopStyleColor();
    ImGui::PopFont();

    ImGui::Dummy(ImVec2(0.0f, 4.0f));
}
