#pragma once

#include <vulkan/vulkan.h>

struct SDL_Window;
union SDL_Event;

namespace Vol
{
class VulkanContext;
}

namespace Vol
{
class ImGuiContext {
  public:
    explicit ImGuiContext(SDL_Window *window, VulkanContext *vulkan_context);
    ~ImGuiContext();

    void process_event(SDL_Event *event);
    void render();
    void end_frame();

  private:
    void init_backends(SDL_Window *window);

  private:
    VulkanContext *vulkan_context;
    VkDescriptorPool descriptor_pool;
};
}  // namespace Vol