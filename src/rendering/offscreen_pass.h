#pragma once

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

#include <vector>

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
  private:
    struct Transformations {
        glm::mat4 view;
        glm::mat4 proj;
    };

  public:
    explicit OffscreenPass(
        VulkanContext *context,
        uint32_t width,
        uint32_t height);
    ~OffscreenPass();

    void record(VkCommandBuffer command_buffer, uint32_t frame_index);

    void framebuffer_size_changed(uint32_t width, uint32_t height);

    inline VkSampler get_sampler() const { return sampler; }
    inline VkImageView get_image_view() const { return color.image_view; }

  private:
    void create_color_attachment();
    void create_depth_attachment();
    void create_render_pass();
    void create_framebuffer();
    void create_descriptor_set_layout();
    void create_pipeline();
    void create_vertex_buffer();
    void create_index_buffer();
    void create_uniform_buffers();
    void create_descriptor_pool();
    void create_descriptor_sets();

    void update_uniform_buffer(uint32_t frame_index);

    void destroy_image();

    void create_buffer(
        VkDeviceSize size,
        VkBufferUsageFlags usage,
        VkMemoryPropertyFlags properties,
        VkBuffer &buffer,
        VkDeviceMemory &buffer_memory);
    void copy_buffer(VkBuffer src, VkBuffer dst, VkDeviceSize size);

  private:
    VulkanContext *context;
    uint32_t width, height;
    VkFramebuffer framebuffer = VK_NULL_HANDLE;
    FramebufferAttachment color{}, depth{};
    VkRenderPass render_pass = VK_NULL_HANDLE;
    VkSampler sampler = VK_NULL_HANDLE;

    VkPipelineLayout pipeline_layout = VK_NULL_HANDLE;
    VkPipeline graphics_pipeline = VK_NULL_HANDLE;

    VkBuffer vertex_buffer = VK_NULL_HANDLE;
    VkDeviceMemory vertex_buffer_memory = VK_NULL_HANDLE;
    VkBuffer index_buffer = VK_NULL_HANDLE;
    VkDeviceMemory index_buffer_memory = VK_NULL_HANDLE;
    std::vector<VkBuffer> uniform_buffers;
    std::vector<VkDeviceMemory> uniform_buffers_memory;
    std::vector<void *> uniform_buffers_mapped;

    VkDescriptorSetLayout descriptor_set_layout = VK_NULL_HANDLE;
    VkDescriptorPool descriptor_pool = VK_NULL_HANDLE;
    std::vector<VkDescriptorSet> descriptor_sets;
};
}  // namespace Vol::Rendering