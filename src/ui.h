#pragma once

#include <string>

namespace Vol {
class UI {
  public:
    UI();
    void update();

  private:
    void update_main_menu_bar();
    void update_status_bar();
    void update_main_window();
    void update_viewport();
    void update_controls();

    void slider(const std::string &label, float *v, float v_min, float v_max);
};
}  // namespace Vol