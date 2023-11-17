#pragma once

#include <vulkan/vulkan.h>

namespace Vol::Rendering
{
class VulkanContext;
}

namespace Vol::Rendering
{
struct FramebufferAttachment {
    VkFormat format;
    VkImage image = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkImageView image_view = VK_NULL_HANDLE;
};

class OffscreenPass {
  public:
    explicit OffscreenPass(
        VulkanContext *context,
        uint32_t width,
        uint32_t height);
    ~OffscreenPass();

    void record(VkCommandBuffer command_buffer);

    void framebuffer_size_changed(uint32_t width, uint32_t height);

    inline VkSemaphore get_semaphore() const { return semaphore; }
    inline VkSampler get_sampler() const { return sampler; }
    inline VkImageView get_image_view() const { return color.image_view; }

  private:
    void create_color_attachment();
    void create_depth_attachment();
    void create_render_pass();
    void create_framebuffer();
    void create_pipeline();
    void create_vertex_buffer();
    void create_sync_objects();

    void destroy_image();

  private:
    VulkanContext *context;
    uint32_t width, height;
    VkFramebuffer framebuffer = VK_NULL_HANDLE;
    FramebufferAttachment color{}, depth{};
    VkSemaphore semaphore;
    VkRenderPass render_pass = VK_NULL_HANDLE;
    VkSampler sampler = VK_NULL_HANDLE;

    VkPipelineLayout pipeline_layout = VK_NULL_HANDLE;
    VkPipeline graphics_pipeline = VK_NULL_HANDLE;
    VkBuffer vertex_buffer = VK_NULL_HANDLE;
    VkDeviceMemory vertex_buffer_memory = VK_NULL_HANDLE;
};
}  // namespace Vol::Rendering