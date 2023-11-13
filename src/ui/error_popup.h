#pragma once

#include <string>

namespace Vol::UI
{
class ErrorPopup {
  public:
    void update();

    void show(const std::string &title, const std::string &message);

  private:
    std::string title;
    std::string message;
};
}  // namespace Vol::UI