#pragma once

#include <vulkan/vulkan.h>

struct SDL_Window;

namespace Vol {
class VulkanContext;

class ImGuiContext {
  public:
    explicit ImGuiContext(SDL_Window *window, VulkanContext *vulkan_context);
    ~ImGuiContext();

    void render();

  private:
    void init_backends(SDL_Window *window);

  private:
    VulkanContext *vulkan_context;
    VkDescriptorPool descriptor_pool;
};
}  // namespace Vol