#pragma once

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

#include <vector>

namespace Vol::Data
{
struct Dataset;
}

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
    struct UniformBufferObject {
        alignas(16) glm::mat4 view;
        alignas(16) glm::mat4 proj;
        alignas(16) glm::vec3 camera_position;
        alignas(4) float min_density;
        alignas(4) float max_density;
        alignas(16) glm::vec3 min_slice = glm::vec3(0.0f);
        alignas(16) glm::vec3 max_slice = glm::vec3(1.0f);
    };

  public:
    explicit OffscreenPass(
        VulkanContext *context,
        uint32_t width,
        uint32_t height);
    ~OffscreenPass();

    void record(VkCommandBuffer command_buffer, uint32_t frame_index);

    void framebuffer_size_changed(uint32_t width, uint32_t height);
    void volume_dataset_changed(Vol::Data::Dataset &dataset);
    void slicing_changed(const glm::vec3 &min, const glm::vec3 &max);
    void transfer_function_changed(const std::vector<glm::uint32_t> &data);

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
    void create_volume(Vol::Data::Dataset &dataset);
    void create_volume_image(Vol::Data::Dataset &dataset);
    void create_volume_image_view();
    void create_volume_sampler();
    void create_transfer(const std::vector<glm::uint32_t> &data);
    void create_transfer_image(const std::vector<glm::uint32_t> &data);
    void create_transfer_image_view();
    void create_transfer_sampler();

    void update_uniform_buffer(uint32_t frame_index);
    void update_descriptor_sets();

    void destroy_image();
    void destroy_volume();
    void destroy_transfer();

    void create_buffer(
        VkDeviceSize size,
        VkBufferUsageFlags usage,
        VkMemoryPropertyFlags properties,
        VkBuffer &buffer,
        VkDeviceMemory &buffer_memory);

    void create_image(
        VkImageType image_type,
        VkFormat format,
        VkExtent3D extent,
        VkImageTiling tiling,
        VkImageUsageFlags usage,
        VkMemoryPropertyFlags properties,
        VkImage &image,
        VkDeviceMemory &image_memory);

    void copy_buffer(VkBuffer src, VkBuffer dst, VkDeviceSize size);
    void copy_buffer_to_image(VkBuffer src, VkImage dst, VkExtent3D extent);

    void transition_image_layout(
        VkImage image,
        VkFormat format,
        VkImageLayout old_layout,
        VkImageLayout new_layout);

  private:
    VulkanContext *context;
    uint32_t width, height;
    VkFramebuffer framebuffer = VK_NULL_HANDLE;
    FramebufferAttachment color{}, depth{};
    VkSampler sampler = VK_NULL_HANDLE;
    VkRenderPass render_pass = VK_NULL_HANDLE;

    VkPipelineLayout pipeline_layout = VK_NULL_HANDLE;
    VkPipeline graphics_pipeline = VK_NULL_HANDLE;

    VkDescriptorSetLayout descriptor_set_layout = VK_NULL_HANDLE;
    VkDescriptorPool descriptor_pool = VK_NULL_HANDLE;
    std::vector<VkDescriptorSet> descriptor_sets;

    VkBuffer vertex_buffer = VK_NULL_HANDLE;
    VkDeviceMemory vertex_buffer_memory = VK_NULL_HANDLE;
    VkBuffer index_buffer = VK_NULL_HANDLE;
    VkDeviceMemory index_buffer_memory = VK_NULL_HANDLE;

    std::vector<VkBuffer> uniform_buffers;
    std::vector<VkDeviceMemory> uniform_buffers_memory;
    std::vector<void *> uniform_buffers_mapped;

    UniformBufferObject ubo;

    VkImage volume_image = VK_NULL_HANDLE;
    VkDeviceMemory volume_image_memory = VK_NULL_HANDLE;
    VkImageView volume_image_view = VK_NULL_HANDLE;
    VkSampler volume_sampler = VK_NULL_HANDLE;

    VkImage transfer_image = VK_NULL_HANDLE;
    VkDeviceMemory transfer_image_memory = VK_NULL_HANDLE;
    VkImageView transfer_image_view = VK_NULL_HANDLE;
    VkSampler transfer_sampler = VK_NULL_HANDLE;

    float min_density = 0.0f;
    float max_density = 255.0f;
};
}  // namespace Vol::Rendering