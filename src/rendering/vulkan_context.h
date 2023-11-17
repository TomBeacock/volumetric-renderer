#pragma once

#include <vulkan/vulkan.h>

#include <vector>

struct SDL_Window;

namespace Vol::Rendering
{
class MainPass;
class OffscreenPass;
}  // namespace Vol::Rendering

namespace Vol::Rendering
{
class VulkanContext {
  public:
    explicit VulkanContext(SDL_Window *window);
    ~VulkanContext();

    void render();
    void wait_till_idle();

    inline VkInstance get_instance() const { return instance; }
    inline VkSurfaceKHR get_surface() const { return surface; }
    inline SDL_Window *const get_window() const { return window; }
    inline VkPhysicalDevice get_physical_device() const
    {
        return physical_device;
    }
    inline VkDevice get_device() const { return device; }
    inline VkQueue get_graphics_queue() const { return graphics_queue; }
    inline VkQueue get_present_queue() const { return present_queue; }
    inline VkCommandPool get_command_pool() const { return command_pool; }

    inline MainPass *const get_main_pass() const { return main_pass; }
    inline OffscreenPass *const get_offscreen_pass() { return offscreen_pass; }

  private:
    void create_instance();
    void create_surface();
    void select_physical_device();
    void create_device();
    void create_command_pool();

    void record_command_buffer(
        VkCommandBuffer command_buffer,
        uint32_t image_index);

  private:
    SDL_Window *window;
    MainPass *main_pass;
    OffscreenPass *offscreen_pass;
    VkInstance instance = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkPhysicalDevice physical_device = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkQueue graphics_queue = VK_NULL_HANDLE;
    VkQueue present_queue = VK_NULL_HANDLE;
    VkCommandPool command_pool = VK_NULL_HANDLE;
};
}  // namespace Vol::Rendering