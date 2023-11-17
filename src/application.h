#pragma once

struct SDL_Window;

namespace Vol
{
class VulkanContext;
class ImGuiContext;
}  // namespace Vol

namespace Vol::Rendering
{
class VulkanContext;
}

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

    inline Rendering::VulkanContext &get_vulkan_context() const
    {
        return *vulkan_context;
    }
    inline ImGuiContext &get_imgui_context() const { return *imgui_context; }
    inline const Data::Importer &get_importer() const { return *importer; };
    inline UI::UIContext &get_ui() { return *ui_context; }

  public:
    static Application &main();

  private:
    bool running;
    SDL_Window *window;
    Rendering::VulkanContext *vulkan_context;
    ImGuiContext *imgui_context;
    UI::UIContext *ui_context;
    Data::Importer *importer;

  private:
    static Application *instance;
};
}  // namespace Vol