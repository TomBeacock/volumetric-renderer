#pragma once

#include <vulkan/vulkan.h>

#include <vector>

namespace Vol::Rendering
{
class VulkanContext;
}

namespace Vol::Rendering
{
class MainPass {
  private:
    struct SwapChain {
        VkSwapchainKHR handle = VK_NULL_HANDLE;
        VkFormat format = VK_FORMAT_UNDEFINED;
        VkExtent2D extent = {};
        std::vector<VkImage> images;
        std::vector<VkImageView> image_views;
        std::vector<VkFramebuffer> frame_buffers;
    };

  public:
    explicit MainPass(VulkanContext *context);
    ~MainPass();

    void render();

    void framebuffer_size_changed();

    inline const SwapChain &get_swap_chain() const { return swap_chain; }
    inline VkRenderPass get_render_pass() const { return render_pass; }
    inline uint32_t get_frame_index() const { return frame_index; }

  private:
    void create_swap_chain();
    void create_image_views();
    void create_render_pass();
    void create_framebuffers();
    void create_sync_objects();
    void allocate_command_buffers();

    void recreate_swap_chain();

    void destroy_swap_chain();

    void record(VkCommandBuffer command_buffer);

  private:
    VulkanContext *context;
    SwapChain swap_chain;
    VkRenderPass render_pass = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> command_buffers;
    std::vector<VkSemaphore> image_available_semaphores;
    std::vector<VkSemaphore> render_finished_semaphores;
    std::vector<VkFence> in_flight_fences;
    uint32_t frame_index = 0;
    uint32_t image_index = 0;
    bool framebuffer_size_dirty = false;
};
}  // namespace Vol::Rendering