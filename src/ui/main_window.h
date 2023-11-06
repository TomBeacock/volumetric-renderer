#pragma once

#include <string>

#include "raw_importer.h"

namespace Vol::UI {
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
        const std::string &hint
    );

  private:
    std::string status_text;
    RawImporter raw_importer;
};
}  // namespace Vol::UI