#pragma once

#include "imgui.h"

#include <filesystem>
#include <optional>
#include <string>

namespace Vol::UI
{
class MainWindow {
  public:
    void update();

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
    uint32_t viewport_width = 0, viewport_height = 0;
};
}  // namespace Vol::UI