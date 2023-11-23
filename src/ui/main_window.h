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

    void set_status_text_on_hover(const std::string &text);

    void slider(
        const std::string &label,
        float *v,
        float v_min,
        float v_max,
        const std::string &hint);

    std::optional<std::filesystem::path> open_file_dialog() const;

  private:
    std::string status_text = "";
    double framerate = 0.0;
    glm::u32vec2 current_scene_window_size;
};
}  // namespace Vol::UI