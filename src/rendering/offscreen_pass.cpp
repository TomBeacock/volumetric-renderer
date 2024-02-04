#include "offscreen_pass.h"

#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include "application.h"
#include "data/dataset.h"
#include "rendering/main_pass.h"
#include "rendering/util.h"
#include "rendering/vulkan_context.h"
#include "scene/scene.h"

#include <backends/imgui_impl_vulkan.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <array>
#include <fstream>
#include <stdexcept>

#define RES(path) RES_PATH path

struct Vertex {
    glm::vec3 position;
    glm::vec3 tex_coord;

    static VkVertexInputBindingDescription get_binding_description()
    {
        return {
            .binding = 0,
            .stride = sizeof(Vertex),
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
        };
    }

    static std::array<VkVertexInputAttributeDescription, 2>
    get_attribute_descriptions()
    {
        return {
            VkVertexInputAttributeDescription{
                .location = 0,
                .binding = 0,
                .format = VK_FORMAT_R32G32B32_SFLOAT,
                .offset = offsetof(Vertex, position),
            },
            VkVertexInputAttributeDescription{
                .location = 1,
                .binding = 0,
                .format = VK_FORMAT_R32G32B32_SFLOAT,
                .offset = offsetof(Vertex, tex_coord),
            },
        };
    }
};

const std::vector<Vertex> vertices = {
    // Top
    {{-0.5f, -0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
    {{0.5f, -0.5f, 0.5f}, {1.0f, 0.0f, 1.0f}},
    {{0.5f, 0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}},
    {{-0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 1.0f}},
    // Front
    {{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, 0.0f}},
    {{0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
    {{0.5f, -0.5f, 0.5f}, {1.0f, 0.0f, 1.0f}},
    {{-0.5f, -0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
    // Right
    {{0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
    {{0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 0.0f}},
    {{0.5f, 0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}},
    {{0.5f, -0.5f, 0.5f}, {1.0f, 0.0f, 1.0f}},
    // Back
    {{0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 0.0f}},
    {{-0.5f, 0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
    {{-0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 1.0f}},
    {{0.5f, 0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}},
    // Left
    {{-0.5f, 0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
    {{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, 0.0f}},
    {{-0.5f, -0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
    {{-0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 1.0f}},
    // Bottom
    {{-0.5f, 0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
    {{0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 0.0f}},
    {{0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
    {{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, 0.0f}},
};

const std::vector<uint16_t> indices = {
    0,  1,  2,  0,  2,  3,  4,  5,  6,  4,  6,  7,  8,  9,  10, 8,  10, 11,
    12, 13, 14, 12, 14, 15, 16, 17, 18, 16, 18, 19, 20, 21, 22, 20, 22, 23};

uint32_t get_memory_type_index(
    Vol::Rendering::VulkanContext *context,
    uint32_t type_bits,
    VkMemoryPropertyFlags properties);

VkBool32 get_supported_depth_format(
    VkPhysicalDevice physical_device,
    VkFormat *depth_format);

std::vector<char> read_binary_file(const std::string &filename);

VkShaderModule create_shader_module(
    VkDevice device,
    const std::vector<char> &code);

uint32_t find_memory_type(
    VkPhysicalDevice physical_device,
    uint32_t type_filter,
    VkMemoryPropertyFlags properties);

Vol::Rendering::OffscreenPass::OffscreenPass(
    VulkanContext *context,
    uint32_t width,
    uint32_t height)
    : context(context), width(width), height(height)
{
    Vol::Data::Dataset temp_volume{{1, 1, 1}, 0.0f, 1.0f, {0.0f}};
    std::vector<glm::uint32_t> temp_transfer{0xFFFFFFFF};

    create_color_attachment();
    create_depth_attachment();
    create_render_pass();
    create_framebuffer();
    create_descriptor_set_layout();
    create_pipeline();
    create_vertex_buffer();
    create_index_buffer();
    create_uniform_buffers();
    create_volume(temp_volume);
    create_transfer(temp_transfer);
    create_descriptor_pool();
    create_descriptor_sets();
}

Vol::Rendering::OffscreenPass::~OffscreenPass()
{
    vkDestroyBuffer(context->get_device(), vertex_buffer, nullptr);
    vkFreeMemory(context->get_device(), vertex_buffer_memory, nullptr);

    vkDestroyBuffer(context->get_device(), index_buffer, nullptr);
    vkFreeMemory(context->get_device(), index_buffer_memory, nullptr);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroyBuffer(context->get_device(), uniform_buffers[i], nullptr);
        vkFreeMemory(context->get_device(), uniform_buffers_memory[i], nullptr);
    }

    destroy_volume();

    vkDestroyDescriptorPool(context->get_device(), descriptor_pool, nullptr);
    vkDestroyDescriptorSetLayout(
        context->get_device(), descriptor_set_layout, nullptr);

    vkDestroyPipelineLayout(context->get_device(), pipeline_layout, nullptr);
    vkDestroyPipeline(context->get_device(), graphics_pipeline, nullptr);

    destroy_image();
    vkDestroyRenderPass(context->get_device(), render_pass, nullptr);
}

void Vol::Rendering::OffscreenPass::record(
    VkCommandBuffer command_buffer,
    uint32_t frame_index)
{
    update_uniform_buffer(context->get_main_pass()->get_frame_index());

    // Define clear colors
    std::array<VkClearValue, 2> clear_values = {
        VkClearValue{.color = {0.11f, 0.11f, 0.11f, 1.0f}},
        VkClearValue{.depthStencil = {1.0f, 0}},
    };

    // Begin render pass
    VkRenderPassBeginInfo render_pass_begin_info = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = render_pass,
        .framebuffer = framebuffer,
        .renderArea =
            VkRect2D{.extent = VkExtent2D{.width = width, .height = height}},
        .clearValueCount = static_cast<uint32_t>(clear_values.size()),
        .pClearValues = clear_values.data(),
    };

    vkCmdBeginRenderPass(
        command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

    // Set viewport
    VkViewport viewport = {
        .x = 0,
        .y = 0,
        .width = static_cast<float>(width),
        .height = static_cast<float>(height),
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };
    vkCmdSetViewport(command_buffer, 0, 1, &viewport);

    // Set scissor
    VkRect2D scissor = {
        .offset = VkOffset2D{0, 0},
        .extent = VkExtent2D{width, height},
    };
    vkCmdSetScissor(command_buffer, 0, 1, &scissor);

    // Bind pipeline
    vkCmdBindPipeline(
        command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline);

    // Bind buffers
    VkBuffer vertex_buffers[] = {vertex_buffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(command_buffer, 0, 1, vertex_buffers, offsets);

    // Bind index buffer
    vkCmdBindIndexBuffer(command_buffer, index_buffer, 0, VK_INDEX_TYPE_UINT16);

    // Bind uniform buffer
    vkCmdBindDescriptorSets(
        command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1,
        &descriptor_sets[frame_index], 0, nullptr);

    // Draw
    vkCmdDrawIndexed(
        command_buffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);

    // End render pass
    vkCmdEndRenderPass(command_buffer);
}

void Vol::Rendering::OffscreenPass::framebuffer_size_changed(
    uint32_t width,
    uint32_t height)
{
    // Ignore if size is zero
    if (width == 0 || height == 0) {
        return;
    }

    // Wait till device is available
    context->wait_till_idle();

    // Update size
    this->width = width;
    this->height = height;

    // Destroy old image information
    destroy_image();

    // Create new image information
    create_color_attachment();
    create_depth_attachment();
    create_framebuffer();
}

void Vol::Rendering::OffscreenPass::volume_dataset_changed(
    Vol::Data::Dataset &dataset)
{
    context->wait_till_idle();

    destroy_volume();
    create_volume(dataset);

    this->ubo.min_density = dataset.min;
    this->ubo.max_density = dataset.max;

    update_descriptor_sets();
}

void Vol::Rendering::OffscreenPass::slicing_changed(
    const glm::vec3 &min,
    const glm::vec3 &max)
{
    this->ubo.min_slice = min;
    this->ubo.max_slice = max;
}

void Vol::Rendering::OffscreenPass::transfer_function_changed(
    const std::vector<glm::uint32_t> &data)
{
    context->wait_till_idle();

    destroy_transfer();
    create_transfer(data);

    update_descriptor_sets();
}

void Vol::Rendering::OffscreenPass::create_color_attachment()
{
    // Define format
    color.format = VK_FORMAT_R8G8B8A8_UNORM;

    // Create image
    VkImageCreateInfo image_create_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = color.format,
        .extent = VkExtent3D{.width = width, .height = height, .depth = 1},
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage =
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
    };

    if (vkCreateImage(
            context->get_device(), &image_create_info, nullptr, &color.image) !=
        VK_SUCCESS) {
        throw std::runtime_error("Failed to create image");
    }

    // Allocate memory
    VkMemoryRequirements memory_requirements;
    vkGetImageMemoryRequirements(
        context->get_device(), color.image, &memory_requirements);

    VkMemoryAllocateInfo memory_alloc_info = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memory_requirements.size,
        .memoryTypeIndex = get_memory_type_index(
            context, memory_requirements.memoryTypeBits,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
    };

    if (vkAllocateMemory(
            context->get_device(), &memory_alloc_info, nullptr,
            &color.memory) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate memory");
    }

    if (vkBindImageMemory(
            context->get_device(), color.image, color.memory, 0) !=
        VK_SUCCESS) {
        throw std::runtime_error("Failed to bind image memory");
    }

    // Create image view
    VkImageViewCreateInfo image_view_create_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = color.image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = color.format,
        .subresourceRange =
            VkImageSubresourceRange{
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
    };

    if (vkCreateImageView(
            context->get_device(), &image_view_create_info, nullptr,
            &color.image_view) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create image view");
    }

    // Create sampler
    VkSamplerCreateInfo sampler_create_info = {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter = VK_FILTER_LINEAR,
        .minFilter = VK_FILTER_LINEAR,
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .mipLodBias = 0.0f,
        .maxAnisotropy = 1.0f,
        .minLod = 0.0f,
        .maxLod = 1.0f,
        .borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE,
    };

    if (vkCreateSampler(
            context->get_device(), &sampler_create_info, nullptr, &sampler) !=
        VK_SUCCESS) {
        throw std::runtime_error("Failed to create sampler");
    }
}

void Vol::Rendering::OffscreenPass::create_depth_attachment()
{
    // Get depth format
    if (!get_supported_depth_format(
            context->get_physical_device(), &depth.format)) {
        throw std::runtime_error("Failed to get depth format");
    }

    // Create image
    VkImageCreateInfo image_create_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = depth.format,
        .extent = VkExtent3D{.width = width, .height = height, .depth = 1},
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
    };

    if (vkCreateImage(
            context->get_device(), &image_create_info, nullptr, &depth.image) !=
        VK_SUCCESS) {
        throw std::runtime_error("Failed to create image");
    }

    // Allocate memory
    VkMemoryRequirements memory_requirements;
    vkGetImageMemoryRequirements(
        context->get_device(), depth.image, &memory_requirements);

    VkMemoryAllocateInfo memory_alloc_info = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memory_requirements.size,
        .memoryTypeIndex = get_memory_type_index(
            context, memory_requirements.memoryTypeBits,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
    };

    if (vkAllocateMemory(
            context->get_device(), &memory_alloc_info, nullptr,
            &depth.memory) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate memory");
    }

    if (vkBindImageMemory(
            context->get_device(), depth.image, depth.memory, 0) !=
        VK_SUCCESS) {
        throw std::runtime_error("Failed to bind image memory");
    }

    // Create image view
    VkImageViewCreateInfo image_view_create_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = depth.image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = depth.format,
        .subresourceRange =
            VkImageSubresourceRange{
                .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
    };

    if (depth.format >= VK_FORMAT_D16_UNORM_S8_UINT) {
        image_view_create_info.subresourceRange.aspectMask |=
            VK_IMAGE_ASPECT_STENCIL_BIT;
    }

    if (vkCreateImageView(
            context->get_device(), &image_view_create_info, nullptr,
            &depth.image_view) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create image view");
    }
}

void Vol::Rendering::OffscreenPass::create_render_pass()
{
    // Define attachment descriptions
    std::array<VkAttachmentDescription, 2> attachment_descriptions = {
        VkAttachmentDescription{
            .format = color.format,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        },
        VkAttachmentDescription{
            .format = depth.format,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        },
    };

    // Define attachment references
    VkAttachmentReference color_reference = {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };

    VkAttachmentReference depth_reference = {
        .attachment = 1,
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    };

    // Define subpass
    VkSubpassDescription subpass_description = {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &color_reference,
        .pDepthStencilAttachment = &depth_reference,
    };

    // Define subpass dependancies
    std::array<VkSubpassDependency, 2> dependancies = {
        VkSubpassDependency{
            .srcSubpass = VK_SUBPASS_EXTERNAL,
            .dstSubpass = 0,
            .srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .srcAccessMask = VK_ACCESS_SHADER_READ_BIT,
            .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            .dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT,
        },
        VkSubpassDependency{
            .srcSubpass = 0,
            .dstSubpass = VK_SUBPASS_EXTERNAL,
            .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
            .dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT,
        },
    };

    // Create render pass
    VkRenderPassCreateInfo render_pass_create_info = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount =
            static_cast<uint32_t>(attachment_descriptions.size()),
        .pAttachments = attachment_descriptions.data(),
        .subpassCount = 1,
        .pSubpasses = &subpass_description,
        .dependencyCount = static_cast<uint32_t>(dependancies.size()),
        .pDependencies = dependancies.data(),
    };

    if (vkCreateRenderPass(
            context->get_device(), &render_pass_create_info, nullptr,
            &render_pass) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create render pass");
    }
}

void Vol::Rendering::OffscreenPass::create_framebuffer()
{
    // Create framebuffer
    std::array<VkImageView, 2> image_views = {
        color.image_view, depth.image_view};

    VkFramebufferCreateInfo framebuffer_create_info = {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = render_pass,
        .attachmentCount = static_cast<uint32_t>(image_views.size()),
        .pAttachments = image_views.data(),
        .width = width,
        .height = height,
        .layers = 1,
    };

    if (vkCreateFramebuffer(
            context->get_device(), &framebuffer_create_info, nullptr,
            &framebuffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create framebuffer");
    }
}

void Vol::Rendering::OffscreenPass::create_descriptor_set_layout()
{
    std::array<VkDescriptorSetLayoutBinding, 3> bindings = {
        // Uniform buffer
        VkDescriptorSetLayoutBinding{
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags =
                VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            .pImmutableSamplers = nullptr,
        },
        // Volume texture
        VkDescriptorSetLayoutBinding{
            .binding = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
            .pImmutableSamplers = nullptr,
        },
        VkDescriptorSetLayoutBinding{
            .binding = 2,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
            .pImmutableSamplers = nullptr,
        },
    };

    VkDescriptorSetLayoutCreateInfo layout_create_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = static_cast<uint32_t>(bindings.size()),
        .pBindings = bindings.data(),
    };

    if (vkCreateDescriptorSetLayout(
            context->get_device(), &layout_create_info, nullptr,
            &descriptor_set_layout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create descriptor set layout");
    }
}

void Vol::Rendering::OffscreenPass::create_pipeline()
{
    // Read SPIR-V files
    auto vert_code = read_binary_file(RES("shaders/volume_vert.spv"));
    auto frag_code = read_binary_file(RES("shaders/volume_frag.spv"));

    // Create shader modules
    VkShaderModule vert_module =
        create_shader_module(context->get_device(), vert_code);
    VkShaderModule frag_module =
        create_shader_module(context->get_device(), frag_code);

    // Define shader stage creation information
    VkPipelineShaderStageCreateInfo vert_stage_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_VERTEX_BIT,
        .module = vert_module,
        .pName = "main",
    };

    VkPipelineShaderStageCreateInfo frag_stage_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
        .module = frag_module,
        .pName = "main",
    };

    VkPipelineShaderStageCreateInfo shader_stages[] = {
        vert_stage_create_info,
        frag_stage_create_info,
    };

    // Define pipeline vertex input creation information
    auto binding_description = Vertex::get_binding_description();
    auto attribute_descriptions = Vertex::get_attribute_descriptions();

    VkPipelineVertexInputStateCreateInfo vertex_input_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &binding_description,
        .vertexAttributeDescriptionCount =
            static_cast<uint32_t>(attribute_descriptions.size()),
        .pVertexAttributeDescriptions = attribute_descriptions.data(),
    };

    // Define pipeline assembly creation information
    VkPipelineInputAssemblyStateCreateInfo input_assembly_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE,
    };

    // Define pipeline viewport creation information
    VkPipelineViewportStateCreateInfo viewport_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .scissorCount = 1,
    };

    // Define rasterizer creation information
    VkPipelineRasterizationStateCreateInfo rasterizer_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .depthBiasEnable = VK_FALSE,
        .depthBiasConstantFactor = 0.0f,
        .depthBiasClamp = 0.0f,
        .depthBiasSlopeFactor = 0.0f,
        .lineWidth = 1.0f,
    };

    // Define multisampling creation information
    VkPipelineMultisampleStateCreateInfo multisampling_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable = VK_FALSE,
        .minSampleShading = 1.0f,
        .pSampleMask = nullptr,
        .alphaToCoverageEnable = VK_FALSE,
        .alphaToOneEnable = VK_FALSE,
    };

    // Define pipeline depth stencil blend state create info
    VkPipelineDepthStencilStateCreateInfo depth_blend_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = true,
        .depthWriteEnable = true,
        .depthCompareOp = VK_COMPARE_OP_LESS,
        .depthBoundsTestEnable = VK_FALSE,
        .stencilTestEnable = VK_FALSE,
        .front = {},
        .back = {},
        .minDepthBounds = 0.0f,
        .maxDepthBounds = 1.0f,
    };

    // Define color blend config
    VkPipelineColorBlendAttachmentState color_blend_attachment = {
        .blendEnable = VK_TRUE,
        .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        .colorBlendOp = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        .alphaBlendOp = VK_BLEND_OP_ADD,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
    };

    // Define pipeline color blend state creation information
    VkPipelineColorBlendStateCreateInfo color_blend_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_COPY,
        .attachmentCount = 1,
        .pAttachments = &color_blend_attachment,
        .blendConstants = {0.0f, 0.0f, 0.0f, 0.0f},
    };

    // Define pipeline dynamic state creation information
    std::vector<VkDynamicState> dynamic_states = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
    };

    VkPipelineDynamicStateCreateInfo dynamic_state_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = static_cast<uint32_t>(dynamic_states.size()),
        .pDynamicStates = dynamic_states.data(),
    };

    // Define pipeline layout creation information
    VkPipelineLayoutCreateInfo pipeline_layout_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = &descriptor_set_layout,
        .pushConstantRangeCount = 0,
        .pPushConstantRanges = nullptr,
    };

    if (vkCreatePipelineLayout(
            context->get_device(), &pipeline_layout_create_info, nullptr,
            &pipeline_layout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create pipeline layout");
    }

    // Define pipeline creation information
    VkGraphicsPipelineCreateInfo pipeline_create_info = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = 2,
        .pStages = shader_stages,
        .pVertexInputState = &vertex_input_create_info,
        .pInputAssemblyState = &input_assembly_create_info,
        .pViewportState = &viewport_create_info,
        .pRasterizationState = &rasterizer_create_info,
        .pMultisampleState = &multisampling_create_info,
        .pDepthStencilState = &depth_blend_create_info,
        .pColorBlendState = &color_blend_create_info,
        .pDynamicState = &dynamic_state_create_info,
        .layout = pipeline_layout,
        .renderPass = render_pass,
        .subpass = 0,
    };

    if (vkCreateGraphicsPipelines(
            context->get_device(), VK_NULL_HANDLE, 1, &pipeline_create_info,
            nullptr, &graphics_pipeline) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create graphics pipeline");
    }

    // Destoy shader modules
    vkDestroyShaderModule(context->get_device(), vert_module, nullptr);
    vkDestroyShaderModule(context->get_device(), frag_module, nullptr);
}

void Vol::Rendering::OffscreenPass::create_vertex_buffer()
{
    VkDeviceSize size = sizeof(Vertex) * vertices.size();

    // Create staging buffer
    VkBuffer staging_buffer;
    VkDeviceMemory staging_buffer_memory;
    create_buffer(
        size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        staging_buffer, staging_buffer_memory);

    // Copy data to staging buffer
    void *data;
    vkMapMemory(
        context->get_device(), staging_buffer_memory, 0, size, 0, &data);
    memcpy(data, vertices.data(), static_cast<size_t>(size));
    vkUnmapMemory(context->get_device(), staging_buffer_memory);

    // Create vertex buffer
    create_buffer(
        size,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertex_buffer,
        vertex_buffer_memory);

    // Copy buffer
    copy_buffer(staging_buffer, vertex_buffer, size);

    // Destroy staging buffer
    vkDestroyBuffer(context->get_device(), staging_buffer, nullptr);
    vkFreeMemory(context->get_device(), staging_buffer_memory, nullptr);
}

void Vol::Rendering::OffscreenPass::create_index_buffer()
{
    VkDeviceSize size = sizeof(uint16_t) * indices.size();

    // Create staging buffer
    VkBuffer staging_buffer;
    VkDeviceMemory staging_buffer_memory;
    create_buffer(
        size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        staging_buffer, staging_buffer_memory);

    // Copy data to staging buffer
    void *data;
    vkMapMemory(
        context->get_device(), staging_buffer_memory, 0, size, 0, &data);
    memcpy(data, indices.data(), static_cast<size_t>(size));
    vkUnmapMemory(context->get_device(), staging_buffer_memory);

    // Create index buffer
    create_buffer(
        size,
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, index_buffer, index_buffer_memory);

    // Copy buffer
    copy_buffer(staging_buffer, index_buffer, size);

    // Destroy staging buffer
    vkDestroyBuffer(context->get_device(), staging_buffer, nullptr);
    vkFreeMemory(context->get_device(), staging_buffer_memory, nullptr);
}

void Vol::Rendering::OffscreenPass::create_uniform_buffers()
{
    VkDeviceSize size = sizeof(UniformBufferObject);
    uniform_buffers.resize(MAX_FRAMES_IN_FLIGHT);
    uniform_buffers_memory.resize(MAX_FRAMES_IN_FLIGHT);
    uniform_buffers_mapped.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        create_buffer(
            size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            uniform_buffers[i], uniform_buffers_memory[i]);
        if (vkMapMemory(
                context->get_device(), uniform_buffers_memory[i], 0, size, 0,
                &uniform_buffers_mapped[i]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to map memory");
        }
    }
}

void Vol::Rendering::OffscreenPass::create_descriptor_pool()
{
    std::array<VkDescriptorPoolSize, 2> pool_sizes{
        VkDescriptorPoolSize{
            .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = MAX_FRAMES_IN_FLIGHT,
        },
        VkDescriptorPoolSize{
            .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = MAX_FRAMES_IN_FLIGHT * 2,
        },
    };

    VkDescriptorPoolCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = MAX_FRAMES_IN_FLIGHT,
        .poolSizeCount = static_cast<uint32_t>(pool_sizes.size()),
        .pPoolSizes = pool_sizes.data(),
    };

    if (vkCreateDescriptorPool(
            context->get_device(), &create_info, nullptr, &descriptor_pool) !=
        VK_SUCCESS) {
        throw std::runtime_error("Failed to create descriptor pool");
    }
}

void Vol::Rendering::OffscreenPass::create_descriptor_sets()
{
    // Allocate descriptor sets
    std::vector<VkDescriptorSetLayout> layouts(
        MAX_FRAMES_IN_FLIGHT, descriptor_set_layout);

    VkDescriptorSetAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = descriptor_pool,
        .descriptorSetCount = MAX_FRAMES_IN_FLIGHT,
        .pSetLayouts = layouts.data(),
    };

    descriptor_sets.resize(MAX_FRAMES_IN_FLIGHT);
    if (vkAllocateDescriptorSets(
            context->get_device(), &alloc_info, descriptor_sets.data()) !=
        VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate descriptor sets");
    }

    update_descriptor_sets();
}

void Vol::Rendering::OffscreenPass::create_volume(Vol::Data::Dataset &dataset)
{
    create_volume_image(dataset);
    create_volume_image_view();
    create_volume_sampler();
}

void Vol::Rendering::OffscreenPass::create_volume_image(
    Vol::Data::Dataset &dataset)
{
    VkDeviceSize size = sizeof(float) * dataset.data.size();

    // Create staging buffer
    VkBuffer staging_buffer;
    VkDeviceMemory staging_buffer_memory;
    create_buffer(
        size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        staging_buffer, staging_buffer_memory);

    // Copy data to staging buffer
    void *data;
    vkMapMemory(
        context->get_device(), staging_buffer_memory, 0, size, 0, &data);
    memcpy(data, dataset.data.data(), static_cast<size_t>(size));
    vkUnmapMemory(context->get_device(), staging_buffer_memory);

    // Create image
    VkExtent3D extent = {
        .width = dataset.dimensions.x,
        .height = dataset.dimensions.y,
        .depth = dataset.dimensions.z,
    };
    create_image(
        VK_IMAGE_TYPE_3D, VK_FORMAT_R32_SFLOAT, extent, VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, volume_image, volume_image_memory);

    // Transition layout
    transition_image_layout(
        volume_image, VK_FORMAT_R32_SFLOAT, VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    // Copy buffer
    copy_buffer_to_image(staging_buffer, volume_image, extent);

    // Transition layout
    transition_image_layout(
        volume_image, VK_FORMAT_R32_SFLOAT,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    // Destroy staging buffer
    vkDestroyBuffer(context->get_device(), staging_buffer, nullptr);
    vkFreeMemory(context->get_device(), staging_buffer_memory, nullptr);
}

void Vol::Rendering::OffscreenPass::create_volume_image_view()
{
    VkImageViewCreateInfo create_info{
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = volume_image,
        .viewType = VK_IMAGE_VIEW_TYPE_3D,
        .format = VK_FORMAT_R32_SFLOAT,
        .subresourceRange =
            VkImageSubresourceRange{
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
    };

    if (vkCreateImageView(
            context->get_device(), &create_info, nullptr, &volume_image_view)) {
        throw std::runtime_error("Failed to create image view");
    }
}

void Vol::Rendering::OffscreenPass::create_volume_sampler()
{
    VkSamplerCreateInfo create_info{
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter = VK_FILTER_LINEAR,
        .minFilter = VK_FILTER_LINEAR,
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
        .mipLodBias = 0.0f,
        .anisotropyEnable = VK_FALSE,
        .maxAnisotropy = 0.0f,
        .compareEnable = VK_FALSE,
        .compareOp = VK_COMPARE_OP_ALWAYS,
        .minLod = 0.0f,
        .maxLod = 0.0f,
        .borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK,
        .unnormalizedCoordinates = VK_FALSE,
    };

    if (vkCreateSampler(
            context->get_device(), &create_info, nullptr, &volume_sampler) !=
        VK_SUCCESS) {
        throw std::runtime_error("Failed to create sampler");
    }
}

void Vol::Rendering::OffscreenPass::create_transfer(
    const std::vector<glm::uint32_t> &data)
{
    create_transfer_image(data);
    create_transfer_image_view();
    create_transfer_sampler();
}

void Vol::Rendering::OffscreenPass::create_transfer_image(
    const std::vector<glm::uint32_t> &data)
{
    VkDeviceSize size = sizeof(glm::uint32_t) * data.size();

    // Create staging buffer
    VkBuffer staging_buffer;
    VkDeviceMemory staging_buffer_memory;
    create_buffer(
        size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        staging_buffer, staging_buffer_memory);

    // Copy data to staging buffer
    void *dst;
    vkMapMemory(context->get_device(), staging_buffer_memory, 0, size, 0, &dst);
    memcpy(dst, data.data(), static_cast<size_t>(size));
    vkUnmapMemory(context->get_device(), staging_buffer_memory);

    // Create image
    VkExtent3D extent = {
        .width = static_cast<uint32_t>(data.size()),
        .height = 1,
        .depth = 1,
    };
    create_image(
        VK_IMAGE_TYPE_1D, VK_FORMAT_R8G8B8A8_SRGB, extent,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, transfer_image,
        transfer_image_memory);

    // Transition layout
    transition_image_layout(
        transfer_image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    // Copy buffer
    copy_buffer_to_image(staging_buffer, transfer_image, extent);

    // Transition layout
    transition_image_layout(
        transfer_image, VK_FORMAT_R8G8B8A8_SRGB,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    // Destroy staging buffer
    vkDestroyBuffer(context->get_device(), staging_buffer, nullptr);
    vkFreeMemory(context->get_device(), staging_buffer_memory, nullptr);
}

void Vol::Rendering::OffscreenPass::create_transfer_image_view()
{
    VkImageViewCreateInfo create_info{
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = transfer_image,
        .viewType = VK_IMAGE_VIEW_TYPE_1D,
        .format = VK_FORMAT_R8G8B8A8_SRGB,
        .subresourceRange =
            VkImageSubresourceRange{
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
    };

    if (vkCreateImageView(
            context->get_device(), &create_info, nullptr,
            &transfer_image_view)) {
        throw std::runtime_error("Failed to create image view");
    }
}

void Vol::Rendering::OffscreenPass::create_transfer_sampler()
{
    VkSamplerCreateInfo create_info{
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter = VK_FILTER_LINEAR,
        .minFilter = VK_FILTER_LINEAR,
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .mipLodBias = 0.0f,
        .anisotropyEnable = VK_FALSE,
        .maxAnisotropy = 0.0f,
        .compareEnable = VK_FALSE,
        .compareOp = VK_COMPARE_OP_ALWAYS,
        .minLod = 0.0f,
        .maxLod = 0.0f,
        .borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK,
        .unnormalizedCoordinates = VK_FALSE,
    };

    if (vkCreateSampler(
            context->get_device(), &create_info, nullptr, &transfer_sampler) !=
        VK_SUCCESS) {
        throw std::runtime_error("Failed to create sampler");
    }
}

void Vol::Rendering::OffscreenPass::update_uniform_buffer(uint32_t frame_index)
{
    float aspect = static_cast<float>(width) / static_cast<float>(height);
    Vol::Scene::Camera &camera = Application::main().get_scene().get_camera();

    // Convert to vulkan coordinate system
    glm::mat4 coordinate_conversion =
        glm::rotate(
            glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)) *
        glm::scale(glm::mat4(1.0f), glm::vec3(-1.0f, 1.0f, 1.0f));

    // Update uniform buffer object
    this->ubo.view = camera.get_view();
    this->ubo.proj =
        glm::perspectiveRH(glm::radians(40.0f), aspect, 0.1f, 10.0f) *
        coordinate_conversion;
    this->ubo.camera_position = camera.get_position();

    memcpy(uniform_buffers_mapped[frame_index], &ubo, sizeof(ubo));
}

void Vol::Rendering::OffscreenPass::update_descriptor_sets()
{
    // Write descriptor sets
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        // Uniform buffer
        VkDescriptorBufferInfo buffer_info{
            .buffer = uniform_buffers[i],
            .offset = 0,
            .range = sizeof(UniformBufferObject),
        };

        // Volume image
        VkDescriptorImageInfo volume_image_info{
            .sampler = volume_sampler,
            .imageView = volume_image_view,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        };

        // Transfer image
        VkDescriptorImageInfo transfer_image_info{
            .sampler = transfer_sampler,
            .imageView = transfer_image_view,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        };

        std::array<VkWriteDescriptorSet, 3> descriptor_writes{
            VkWriteDescriptorSet{
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = descriptor_sets[i],
                .dstBinding = 0,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .pBufferInfo = &buffer_info,
            },
            VkWriteDescriptorSet{
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = descriptor_sets[i],
                .dstBinding = 1,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .pImageInfo = &volume_image_info,
            },
            VkWriteDescriptorSet{
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = descriptor_sets[i],
                .dstBinding = 2,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .pImageInfo = &transfer_image_info,
            },
        };

        vkUpdateDescriptorSets(
            context->get_device(),
            static_cast<uint32_t>(descriptor_writes.size()),
            descriptor_writes.data(), 0, nullptr);
    }
}

void Vol::Rendering::OffscreenPass::destroy_image()
{
    vkDestroyImageView(context->get_device(), color.image_view, nullptr);
    vkDestroyImage(context->get_device(), color.image, nullptr);
    vkFreeMemory(context->get_device(), color.memory, nullptr);

    vkDestroyImageView(context->get_device(), depth.image_view, nullptr);
    vkDestroyImage(context->get_device(), depth.image, nullptr);
    vkFreeMemory(context->get_device(), depth.memory, nullptr);

    vkDestroyFramebuffer(context->get_device(), framebuffer, nullptr);
    vkDestroySampler(context->get_device(), sampler, nullptr);
}

void Vol::Rendering::OffscreenPass::destroy_volume()
{
    vkDestroySampler(context->get_device(), volume_sampler, nullptr);
    vkDestroyImageView(context->get_device(), volume_image_view, nullptr);
    vkDestroyImage(context->get_device(), volume_image, nullptr);
    vkFreeMemory(context->get_device(), volume_image_memory, nullptr);
}

void Vol::Rendering::OffscreenPass::destroy_transfer()
{
    vkDestroySampler(context->get_device(), transfer_sampler, nullptr);
    vkDestroyImageView(context->get_device(), transfer_image_view, nullptr);
    vkDestroyImage(context->get_device(), transfer_image, nullptr);
    vkFreeMemory(context->get_device(), transfer_image_memory, nullptr);
}

void Vol::Rendering::OffscreenPass::create_buffer(
    VkDeviceSize size,
    VkBufferUsageFlags usage,
    VkMemoryPropertyFlags properties,
    VkBuffer &buffer,
    VkDeviceMemory &buffer_memory)
{
    // Define vertex buffer creation info
    VkBufferCreateInfo buffer_create_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };

    // Create vertex buffer
    if (vkCreateBuffer(
            context->get_device(), &buffer_create_info, nullptr, &buffer) !=
        VK_SUCCESS) {
        throw std::runtime_error("Failed to create buffer");
    }

    // Get memory requirements
    VkMemoryRequirements mem_requirements;
    vkGetBufferMemoryRequirements(
        context->get_device(), buffer, &mem_requirements);

    // Define memory allocation information
    VkMemoryAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = mem_requirements.size,
        .memoryTypeIndex = find_memory_type(
            context->get_physical_device(), mem_requirements.memoryTypeBits,
            properties),
    };

    // Allocate memory
    if (vkAllocateMemory(
            context->get_device(), &alloc_info, nullptr, &buffer_memory) !=
        VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate buffer memory");
    }

    // Bind memory
    vkBindBufferMemory(context->get_device(), buffer, buffer_memory, 0);
}

void Vol::Rendering::OffscreenPass::create_image(
    VkImageType image_type,
    VkFormat format,
    VkExtent3D extent,
    VkImageTiling tiling,
    VkImageUsageFlags usage,
    VkMemoryPropertyFlags properties,
    VkImage &image,
    VkDeviceMemory &image_memory)
{
    VkImageCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = image_type,
        .format = format,
        .extent = extent,
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = tiling,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };

    if (vkCreateImage(context->get_device(), &create_info, nullptr, &image) !=
        VK_SUCCESS) {
        throw std::runtime_error("Failed to create image");
    }

    VkMemoryRequirements memory_requirements;
    vkGetImageMemoryRequirements(
        context->get_device(), image, &memory_requirements);

    VkMemoryAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memory_requirements.size,
        .memoryTypeIndex = find_memory_type(
            context->get_physical_device(), memory_requirements.memoryTypeBits,
            properties),
    };

    if (vkAllocateMemory(
            context->get_device(), &alloc_info, nullptr, &image_memory) !=
        VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate memory");
    }

    vkBindImageMemory(context->get_device(), image, image_memory, 0);
}

void Vol::Rendering::OffscreenPass::copy_buffer(
    VkBuffer src,
    VkBuffer dst,
    VkDeviceSize size)
{
    VkCommandBuffer command_buffer = context->begin_single_command();

    VkBufferCopy copy_region = {
        .srcOffset = 0,
        .dstOffset = 0,
        .size = size,
    };
    vkCmdCopyBuffer(command_buffer, src, dst, 1, &copy_region);

    context->end_single_command(command_buffer);
}

void Vol::Rendering::OffscreenPass::copy_buffer_to_image(
    VkBuffer src,
    VkImage dst,
    VkExtent3D extent)
{
    VkCommandBuffer command_buffer = context->begin_single_command();

    VkBufferImageCopy copy_region{
        .bufferOffset = 0,
        .bufferRowLength = 0,
        .bufferImageHeight = 0,
        .imageSubresource =
            VkImageSubresourceLayers{
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .mipLevel = 0,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
        .imageOffset = {0, 0, 0},
        .imageExtent = extent,
    };

    vkCmdCopyBufferToImage(
        command_buffer, src, dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
        &copy_region);

    context->end_single_command(command_buffer);
}

void Vol::Rendering::OffscreenPass::transition_image_layout(
    VkImage image,
    VkFormat format,
    VkImageLayout old_layout,
    VkImageLayout new_layout)
{
    VkCommandBuffer command_buffer = context->begin_single_command();

    VkImageMemoryBarrier barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask = 0,
        .dstAccessMask = 0,
        .oldLayout = old_layout,
        .newLayout = new_layout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image,
        .subresourceRange =
            VkImageSubresourceRange{
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
    };

    VkPipelineStageFlags src_stage, dst_stage;

    if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED &&
        new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        src_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dst_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (
        old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
        new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        src_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        dst_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else {
        throw std::invalid_argument("Unsupported layout transition");
    }

    vkCmdPipelineBarrier(
        command_buffer, src_stage, dst_stage, 0, 0, nullptr, 0, nullptr, 1,
        &barrier);

    context->end_single_command(command_buffer);
}

uint32_t get_memory_type_index(
    Vol::Rendering::VulkanContext *context,
    uint32_t type_bits,
    VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties device_memory_properties;
    vkGetPhysicalDeviceMemoryProperties(
        context->get_physical_device(), &device_memory_properties);

    for (uint32_t i = 0; i < device_memory_properties.memoryTypeCount; i++) {
        if ((type_bits & 1) == 1) {
            if ((device_memory_properties.memoryTypes[i].propertyFlags &
                 properties) == properties) {
                return i;
            }
        }
        type_bits >>= 1;
    }
    return 0;
}

VkBool32 get_supported_depth_format(
    VkPhysicalDevice physical_device,
    VkFormat *depth_format)
{
    // Define formats in accuracy order
    std::vector<VkFormat> formats = {
        VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D16_UNORM_S8_UINT,
        VK_FORMAT_D16_UNORM};

    // Find first available format
    for (auto &format : formats) {
        VkFormatProperties format_props;
        vkGetPhysicalDeviceFormatProperties(
            physical_device, format, &format_props);

        if (format_props.optimalTilingFeatures &
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
            *depth_format = format;
            return true;
        }
    }
    return false;
}

std::vector<char> read_binary_file(const std::string &filename)
{
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + filename);
    }

    size_t file_size = (size_t)file.tellg();
    std::vector<char> buffer(file_size);

    file.seekg(0);
    file.read(buffer.data(), file_size);

    file.close();

    return buffer;
}

VkShaderModule create_shader_module(
    VkDevice device,
    const std::vector<char> &code)
{
    // Define shader module creation information
    VkShaderModuleCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = code.size(),
        .pCode = reinterpret_cast<const uint32_t *>(code.data()),
    };

    // Create shader module
    VkShaderModule shader_module;
    if (vkCreateShaderModule(device, &create_info, nullptr, &shader_module) !=
        VK_SUCCESS) {
        throw std::runtime_error("Failed to create shader module");
    }

    return shader_module;
}

uint32_t find_memory_type(
    VkPhysicalDevice physical_device,
    uint32_t type_filter,
    VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memory_props;
    vkGetPhysicalDeviceMemoryProperties(physical_device, &memory_props);

    for (uint32_t i = 0; i < memory_props.memoryTypeCount; i++) {
        if (type_filter & (1 << i) &&
            (memory_props.memoryTypes[i].propertyFlags & properties) ==
                properties) {
            return i;
        }
    }

    throw std::runtime_error("Failed to find suitable memory type");
}
