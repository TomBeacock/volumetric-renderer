#pragma once

#include "imgui.h"

#include <glm/glm.hpp>

#include <filesystem>
#include <optional>
#include <string>

namespace Vol::UI
{
class MainWindow {
  public:
    void update();

    inline void set_framerate(double framerate)
    {
        this->framerate = framerate;
    };

  private:
    void update_main_menu_bar();
    void update_status_bar();
    void update_main_window();
    void update_viewport();
    void update_controls();

    void update_viewport_rotation(
        const glm::vec2 &min_bound,
        const glm::vec2 &max_bound);
    void update_viewport_zoom();

    void set_status_text_on_hover(const std::string &text);

    void heading(const std::string &text);

    std::optional<std::filesystem::path> open_file_dialog() const;

  private:
    std::string status_text = "";
    double framerate = 0.0;
    glm::u32vec2 current_scene_window_size{};
};
}  // namespace Vol::UI