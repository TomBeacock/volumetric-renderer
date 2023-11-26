#pragma once

#include <memory>

struct SDL_Window;

namespace Vol::Rendering
{
class VulkanContext;
}

namespace Vol::UI
{
class ImGuiContext;
class UIContext;
}  // namespace Vol::UI

namespace Vol::Data
{
class Importer;
}

namespace Vol::Scene
{
class Scene;
}

namespace Vol
{
class Application {
  public:
    explicit Application();
    ~Application();

    int run();

    inline SDL_Window &get_window() { return *window; }
    inline Rendering::VulkanContext &get_vulkan_context() const
    {
        return *vulkan_context;
    }
    inline UI::ImGuiContext &get_imgui_context() const
    {
        return *imgui_context;
    }
    inline UI::UIContext &get_ui() { return *ui_context; }
    inline const Data::Importer &get_importer() const { return *importer; };
    inline Scene::Scene &get_scene() { return *scene; }

  public:
    static Application &main();

  private:
    bool running;
    SDL_Window *window;
    Rendering::VulkanContext *vulkan_context;
    UI::ImGuiContext *imgui_context;
    UI::UIContext *ui_context;
    std::unique_ptr<Data::Importer> importer;
    std::unique_ptr<Scene::Scene> scene;

  private:
    static Application *instance;
};
}  // namespace Vol