#pragma once

#include "error_popup.h"
#include "main_window.h"

#include <string>

namespace Vol::UI
{
class UIContext {
  public:
    UIContext();
    void update();

    void show_error(const std::string &title, const std::string &message);

    inline MainWindow &get_main_window() { return main_window; }

  private:
    MainWindow main_window;
    ErrorPopup error_popup;
};
}  // namespace Vol::UI