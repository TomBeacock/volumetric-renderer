#pragma once

#include <vulkan/vulkan.h>

struct SDL_Window;
union SDL_Event;

namespace Vol::Rendering
{
class VulkanContext;
}

namespace Vol::UI
{
class ImGuiContext {
  public:
    explicit ImGuiContext(
        SDL_Window *window,
        Rendering::VulkanContext *vulkan_context);
    ~ImGuiContext();

    void process_event(SDL_Event *event);
    void end_frame();

    void recreate_viewport_texture(uint32_t width, uint32_t height);

    inline VkDescriptorSet get_descriptor() const { return descriptor; }

  private:
    void init_backends(SDL_Window *window);

  private:
    Rendering::VulkanContext *vulkan_context;
    VkDescriptorPool descriptor_pool;
    VkDescriptorSet descriptor;
};
}  // namespace Vol::UI