#pragma once

struct SDL_Window;

namespace Vol
{
class VulkanContext;
class ImGuiContext;
}  // namespace Vol

namespace Vol::UI
{
class UIContext;
}

namespace Vol::Data
{
class Importer;
}

namespace Vol
{
class Application {
  public:
    explicit Application();
    ~Application();

    int run();

    inline const Data::Importer &get_importer() const { return *importer; };
    inline UI::UIContext &get_ui() { return *ui_context; }

  public:
    static Application &main();

  private:
    bool running;
    SDL_Window *window;
    VulkanContext *vulkan_context;
    ImGuiContext *imgui_context;
    UI::UIContext *ui_context;
    Data::Importer *importer;

  private:
    static Application *instance;
};
}  // namespace Vol