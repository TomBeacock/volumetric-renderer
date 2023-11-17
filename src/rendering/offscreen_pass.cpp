#include "offscreen_pass.h"

#include "rendering/util.h"
#include "rendering/vulkan_context.h"

#include <backends/imgui_impl_vulkan.h>
#include <glm/glm.hpp>

#include <array>
#include <fstream>
#include <stdexcept>

#define RES(path) RES_PATH path

struct Vertex {
    glm::vec2 position;
    glm::vec3 color;

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
                .format = VK_FORMAT_R32G32_SFLOAT,
                .offset = offsetof(Vertex, position),
            },
            VkVertexInputAttributeDescription{
                .location = 1,
                .binding = 0,
                .format = VK_FORMAT_R32G32B32_SFLOAT,
                .offset = offsetof(Vertex, color),
            },
        };
    }
};

const std::vector<Vertex> vertices = {
    {{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
    {{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
    {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
};

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
    create_color_attachment();
    create_depth_attachment();
    create_render_pass();
    create_framebuffer();
    create_pipeline();
    create_vertex_buffer();
    create_sync_objects();
}

Vol::Rendering::OffscreenPass::~OffscreenPass()
{
    vkDestroyBuffer(context->get_device(), vertex_buffer, nullptr);
    vkFreeMemory(context->get_device(), vertex_buffer_memory, nullptr);

    vkDestroySemaphore(context->get_device(), semaphore, nullptr);

    vkDestroyPipeline(context->get_device(), graphics_pipeline, nullptr);
    vkDestroyPipelineLayout(context->get_device(), pipeline_layout, nullptr);

    destroy_image();
    vkDestroyRenderPass(context->get_device(), render_pass, nullptr);
}

void Vol::Rendering::OffscreenPass::record(VkCommandBuffer command_buffer)
{
    // Define clear colors
    std::array<VkClearValue, 2> clear_values = {
        VkClearValue{.color = {0.01096f, 0.01096f, 0.01096f, 1.0f}},
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

    // Draw
    vkCmdDraw(command_buffer, static_cast<uint32_t>(vertices.size()), 1, 0, 0);

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

void Vol::Rendering::OffscreenPass::create_pipeline()
{
    // Read SPIR-V files
    auto vert_code = read_binary_file(RES("shaders/basic_vert.spv"));
    auto frag_code = read_binary_file(RES("shaders/basic_frag.spv"));

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
        .frontFace = VK_FRONT_FACE_CLOCKWISE,
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
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
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
        .setLayoutCount = 0,
        .pSetLayouts = nullptr,
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
    // Define vertex buffer creation info
    VkBufferCreateInfo buffer_create_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = sizeof(Vertex) * vertices.size(),
        .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };

    // Create vertex buffer
    if (vkCreateBuffer(
            context->get_device(), &buffer_create_info, nullptr,
            &vertex_buffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create vertex buffer");
    }

    // Get memory requirements
    VkMemoryRequirements mem_requirements;
    vkGetBufferMemoryRequirements(
        context->get_device(), vertex_buffer, &mem_requirements);

    // Define memory allocation information
    VkMemoryAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = mem_requirements.size,
        .memoryTypeIndex = find_memory_type(
            context->get_physical_device(), mem_requirements.memoryTypeBits,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                VK_MEMORY_PROPERTY_HOST_COHERENT_BIT),
    };

    // Allocate memory
    if (vkAllocateMemory(
            context->get_device(), &alloc_info, nullptr,
            &vertex_buffer_memory) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate vertex buffer memory");
    }

    // Bind memory
    vkBindBufferMemory(
        context->get_device(), vertex_buffer, vertex_buffer_memory, 0);

    // Fill buffer
    void *data;
    vkMapMemory(
        context->get_device(), vertex_buffer_memory, 0, buffer_create_info.size,
        0, &data);
    memcpy(data, vertices.data(), static_cast<size_t>(buffer_create_info.size));
    vkUnmapMemory(context->get_device(), vertex_buffer_memory);
}

void Vol::Rendering::OffscreenPass::create_sync_objects()
{
    // Define semaphore creation info
    VkSemaphoreCreateInfo semaphore_create_info = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    };

    if (vkCreateSemaphore(
            context->get_device(), &semaphore_create_info, nullptr,
            &semaphore) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create semaphores");
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
