#pragma once

#include <SDL3/SDL.h>
#include <vulkan/vulkan.h>

#include <optional>
#include <vector>

struct ImDrawData;

namespace Vol {
class ImGuiContext;

class VulkanContext {
    friend class ImGuiContext;

  private:
    struct SwapChain {
        VkSwapchainKHR handle = VK_NULL_HANDLE;
        std::vector<VkImage> images;
        std::vector<VkImageView> image_views;
        VkFormat format = VK_FORMAT_UNDEFINED;
        VkExtent2D extent = {};
        std::vector<VkFramebuffer> frame_buffers;
    };

  public:
    explicit VulkanContext(SDL_Window *window);
    ~VulkanContext();

    void begin_render_pass();
    void end_render_pass();
    void recreate_swap_chain();
    void wait_till_idle();
    void framebuffer_size_changed();

  private:
    void create_instance();
    void create_surface();
    void select_physical_device();
    void create_device();
    void create_swap_chain();
    void create_image_views();
    void create_render_pass();
    void create_graphics_pipeline();
    void create_framebuffers();
    void create_command_pool();
    void create_vertex_buffer();
    void allocate_command_buffers();
    void create_sync_objects();

    void record_command_buffer(
        VkCommandBuffer command_buffer,
        uint32_t image_index
    );

    void cleanup_swap_chain();

  private:
    SDL_Window *window;
    VkInstance instance = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkPhysicalDevice physical_device = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkQueue graphics_queue = VK_NULL_HANDLE;
    VkQueue present_queue = VK_NULL_HANDLE;
    SwapChain swap_chain;
    VkRenderPass render_pass = VK_NULL_HANDLE;
    VkPipelineLayout pipeline_layout = VK_NULL_HANDLE;
    VkPipeline graphics_pipeline = VK_NULL_HANDLE;
    VkCommandPool command_pool = VK_NULL_HANDLE;
    VkBuffer vertex_buffer = VK_NULL_HANDLE;
    VkDeviceMemory vertex_buffer_memory = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> command_buffers;
    std::vector<VkSemaphore> image_available_semaphores;
    std::vector<VkSemaphore> render_finished_semaphores;
    std::vector<VkFence> in_flight_fences;
    uint32_t frame_index = 0;
    uint32_t image_index = 0;
    bool framebuffer_size_dirty = false;
};
}  // namespace Vol