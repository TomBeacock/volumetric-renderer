#pragma once

#include <string>

#include "main_window.h"

namespace Vol::UI {

class UIContext {
  public:
    UIContext();
    void update();

  private:
    MainWindow main_window;
};
}  // namespace Vol::UI