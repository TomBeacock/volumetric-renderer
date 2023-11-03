#include "vulkan_context.h"

#include <SDL_vulkan.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <fstream>
#include <glm/glm.hpp>
#include <iostream>
#include <limits>
#include <map>
#include <set>
#include <stdexcept>
#include <vector>

#define RES(path) RES_PATH path

#ifdef NDEBUG
const bool enable_validation_layers = false;
const std::vector<const char *> validation_layers = {};
#else
const bool enable_validation_layers = true;
const std::vector<const char *> validation_layers = {
    "VK_LAYER_KHRONOS_validation",
};
#endif

const uint32_t MAX_FRAMES_IN_FLIGHT = 2;

const std::vector<const char *> required_physical_device_extensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
};

struct QueueFamilyIndices {
    std::optional<uint32_t> graphics;
    std::optional<uint32_t> presentation;

    bool is_complete() const { return graphics && presentation; }
};

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> present_modes;
};

struct Vertex {
    glm::vec2 position;
    glm::vec3 color;

    static VkVertexInputBindingDescription get_binding_description() {
        return {
            .binding = 0,
            .stride = sizeof(Vertex),
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
        };
    }

    static std::array<VkVertexInputAttributeDescription, 2>
    get_attribute_descriptions() {
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

bool validation_layers_supported();

std::optional<unsigned int> get_physical_device_ranking(
    VkPhysicalDevice device,
    VkSurfaceKHR surface
);

bool is_physical_device_suitable(VkPhysicalDevice device, VkSurfaceKHR surface);

QueueFamilyIndices get_queue_families(
    VkPhysicalDevice device,
    VkSurfaceKHR surface
);

SwapChainSupportDetails get_swap_chain_support(
    VkPhysicalDevice device,
    VkSurfaceKHR surface
);

VkSurfaceFormatKHR select_swap_surface_format(
    const std::vector<VkSurfaceFormatKHR> &available_formats
);

VkPresentModeKHR select_swap_present_mode(
    const std::vector<VkPresentModeKHR> &available_present_modes
);

VkExtent2D select_swap_extent(
    SDL_Window *window,
    const VkSurfaceCapabilitiesKHR &capabilities
);

std::vector<char> read_binary_file(const std::string &filename);

VkShaderModule create_shader_module(
    VkDevice device,
    const std::vector<char> &code
);

uint32_t find_memory_type(
    VkPhysicalDevice physical_device,
    uint32_t type_filter,
    VkMemoryPropertyFlags properties
);

Vol::VulkanContext::VulkanContext(SDL_Window *window) : window(window) {
    create_instance();
    create_surface();
    select_physical_device();
    create_device();
    create_swap_chain();
    create_image_views();
    create_render_pass();
    create_graphics_pipeline();
    create_framebuffers();
    create_command_pool();
    create_vertex_buffer();
    allocate_command_buffers();
    create_sync_objects();
}

Vol::VulkanContext::~VulkanContext() {
    cleanup_swap_chain();

    vkDestroyBuffer(device, vertex_buffer, nullptr);
    vkFreeMemory(device, vertex_buffer_memory, nullptr);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroyFence(device, in_flight_fences[i], nullptr);
        vkDestroySemaphore(device, image_available_semaphores[i], nullptr);
        vkDestroySemaphore(device, render_finished_semaphores[i], nullptr);
    }

    vkDestroyCommandPool(device, command_pool, nullptr);
    vkDestroyPipeline(device, graphics_pipeline, nullptr);
    vkDestroyPipelineLayout(device, pipeline_layout, nullptr);
    vkDestroyRenderPass(device, render_pass, nullptr);
    vkDestroyDevice(device, nullptr);

    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);
}

void Vol::VulkanContext::begin_render_pass() {
    // Wait to start frame
    vkWaitForFences(
        device, 1, &in_flight_fences[frame_index], VK_TRUE, UINT64_MAX
    );

    // Get image index from swap chain
    VkResult result = vkAcquireNextImageKHR(
        device, swap_chain.handle, UINT64_MAX,
        image_available_semaphores[frame_index], VK_NULL_HANDLE, &image_index
    );

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreate_swap_chain();
        return;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("Failed to acquire swap chain image");
    }

    // Image successfully accquired, reset fence and begin frame
    vkResetFences(device, 1, &in_flight_fences[frame_index]);

    // Reset command buffer
    VkCommandBuffer command_buffer = command_buffers[frame_index];
    vkResetCommandBuffer(command_buffer, 0);

    // Define command buffer begin information
    VkCommandBufferBeginInfo begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = 0,
        .pInheritanceInfo = nullptr,
    };

    // Begin command buffer
    if (vkBeginCommandBuffer(command_buffer, &begin_info) != VK_SUCCESS) {
        throw std::runtime_error("Failed to begin command buffer");
    }

    // Define render pass begin info
    VkClearValue clear_color = {{{0.0f, 0.0f, 0.0f, 1.0f}}};

    VkRenderPassBeginInfo render_pass_begin_info = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = render_pass,
        .framebuffer = swap_chain.frame_buffers[image_index],
        .renderArea = {{0, 0}, swap_chain.extent},
        .clearValueCount = 1,
        .pClearValues = &clear_color,
    };

    vkCmdBeginRenderPass(
        command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE
    );

    vkCmdBindPipeline(
        command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline
    );

    // Set viewport
    VkViewport viewport = {
        .x = 0.0f,
        .y = 0.0f,
        .width = static_cast<float>(swap_chain.extent.width),
        .height = static_cast<float>(swap_chain.extent.height),
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };
    vkCmdSetViewport(command_buffer, 0, 1, &viewport);

    // Define scissor
    VkRect2D scissor = {
        .offset = {0, 0},
        .extent = swap_chain.extent,
    };
    vkCmdSetScissor(command_buffer, 0, 1, &scissor);

    // Bind buffers
    VkBuffer vertex_buffers[] = {vertex_buffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(command_buffer, 0, 1, vertex_buffers, offsets);

    // Draw
    vkCmdDraw(command_buffer, static_cast<uint32_t>(vertices.size()), 1, 0, 0);
}

void Vol::VulkanContext::end_render_pass() {
    VkCommandBuffer command_buffer = command_buffers[frame_index];

    // End render pass
    vkCmdEndRenderPass(command_buffer);

    // End command buffer
    if (vkEndCommandBuffer(command_buffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to end command buffer");
    }

    // Submit command buffer
    VkPipelineStageFlags wait_stages[] = {
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

    VkSubmitInfo submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &image_available_semaphores[frame_index],
        .pWaitDstStageMask = wait_stages,
        .commandBufferCount = 1,
        .pCommandBuffers = &command_buffers[frame_index],
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &render_finished_semaphores[frame_index],
    };

    if (vkQueueSubmit(
            graphics_queue, 1, &submit_info, in_flight_fences[frame_index]
        )) {
        throw std::runtime_error("Failed to submit draw command buffer");
    }

    // Present
    VkPresentInfoKHR present_info = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &render_finished_semaphores[frame_index],
        .swapchainCount = 1,
        .pSwapchains = &swap_chain.handle,
        .pImageIndices = &image_index,
        .pResults = nullptr,
    };

    VkResult result = vkQueuePresentKHR(present_queue, &present_info);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR ||
        framebuffer_size_dirty) {
        framebuffer_size_dirty = false;
        recreate_swap_chain();
    } else if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to present swap chain image");
    }

    // Update frame index
    frame_index = (frame_index + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Vol::VulkanContext::recreate_swap_chain() {
    wait_till_idle();

    cleanup_swap_chain();

    create_swap_chain();
    create_image_views();
    create_framebuffers();
}

void Vol::VulkanContext::wait_till_idle() {
    vkDeviceWaitIdle(device);
}

void Vol::VulkanContext::framebuffer_size_changed() {
    framebuffer_size_dirty = true;
}

void Vol::VulkanContext::create_instance() {
    // Check validation layers
    if (enable_validation_layers && !validation_layers_supported()) {
        throw std::runtime_error("Requested validation layer not available");
    }

    // Define application information
    const VkApplicationInfo app_info = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "Template",
        .applicationVersion = VK_MAKE_VERSION(0, 1, 0),
        .pEngineName = "NaN",
        .engineVersion = VK_MAKE_VERSION(0, 1, 0),
        .apiVersion = VK_API_VERSION_1_0,
    };

    // Query and retrieve required extensions
    unsigned int extension_count;
    SDL_Vulkan_GetInstanceExtensions(&extension_count, nullptr);

    std::vector<const char *> extensions(extension_count);
    SDL_Vulkan_GetInstanceExtensions(&extension_count, extensions.data());

    // Define instance creation information
    VkInstanceCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &app_info,
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = nullptr,
        .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
        .ppEnabledExtensionNames = extensions.data(),
    };

    // Enable validation layers
    if (enable_validation_layers) {
        create_info.enabledLayerCount = validation_layers.size();
        create_info.ppEnabledLayerNames = validation_layers.data();
    }

    // Create vulkan instance
    vkCreateInstance(&create_info, nullptr, &instance);
}

void Vol::VulkanContext::create_surface() {
    if (!SDL_Vulkan_CreateSurface(window, instance, &surface)) {
        throw std::runtime_error("Failed to create surface");
    }
}

void Vol::VulkanContext::select_physical_device() {
    // Query physical device count
    uint32_t device_count;
    vkEnumeratePhysicalDevices(instance, &device_count, nullptr);

    if (device_count == 0) {
        throw std::runtime_error("Failed to find physical device");
    }

    // Retrieve physical devices
    std::vector<VkPhysicalDevice> devices(device_count);
    vkEnumeratePhysicalDevices(instance, &device_count, devices.data());

    // Rank devices
    std::multimap<int, VkPhysicalDevice> device_ranking;
    for (const auto &device : devices) {
        if (auto rank = get_physical_device_ranking(device, surface)) {
            device_ranking.emplace(*rank, device);
        }
    }

    if (device_ranking.empty()) {
        throw std::runtime_error("No suitable devices found");
    }

    // Select top ranking device
    physical_device = device_ranking.rbegin()->second;

#ifndef NDEBUG
    VkPhysicalDeviceProperties device_props;
    vkGetPhysicalDeviceProperties(physical_device, &device_props);
    std::cout << "Physical device: " << device_props.deviceName << std::endl;
#endif  // !NDEBUG
}

void Vol::VulkanContext::create_device() {
    // Get queue indices
    QueueFamilyIndices queue_indices =
        get_queue_families(physical_device, surface);

    // Define queue creation informations
    std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
    std::set<uint32_t> unique_queue_indices = {
        *queue_indices.graphics,
        *queue_indices.presentation,
    };

    float queue_priorities = 1.0f;
    for (uint32_t queue_index : unique_queue_indices) {
        const VkDeviceQueueCreateInfo queue_create_info = {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = queue_index,
            .queueCount = 1,
            .pQueuePriorities = &queue_priorities,
        };
        queue_create_infos.push_back(queue_create_info);
    }

    // Get device features
    VkPhysicalDeviceFeatures device_features;
    vkGetPhysicalDeviceFeatures(physical_device, &device_features);

    // Define device creation information
    VkDeviceCreateInfo device_create_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount =
            static_cast<uint32_t>(unique_queue_indices.size()),
        .pQueueCreateInfos = queue_create_infos.data(),
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = nullptr,
        .enabledExtensionCount =
            static_cast<uint32_t>(required_physical_device_extensions.size()),
        .ppEnabledExtensionNames = required_physical_device_extensions.data(),
        .pEnabledFeatures = &device_features,
    };

    // Enable validation layers
    if (enable_validation_layers) {
        device_create_info.enabledLayerCount = validation_layers.size();
        device_create_info.ppEnabledLayerNames = validation_layers.data();
    }

    // Create device
    if (vkCreateDevice(
            physical_device, &device_create_info, nullptr, &device
        ) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create logical device");
    }

    // Get queues
    vkGetDeviceQueue(device, *queue_indices.graphics, 0, &graphics_queue);
    vkGetDeviceQueue(device, *queue_indices.presentation, 0, &present_queue);
}

void Vol::VulkanContext::create_swap_chain() {
    SwapChainSupportDetails swap_chain_support =
        get_swap_chain_support(physical_device, surface);

    // Select swap chain details
    VkSurfaceFormatKHR surface_format =
        select_swap_surface_format(swap_chain_support.formats);
    VkPresentModeKHR present_mode =
        select_swap_present_mode(swap_chain_support.present_modes);
    VkExtent2D extent =
        select_swap_extent(window, swap_chain_support.capabilities);

    // Select swap chain image count
    uint32_t image_count = swap_chain_support.capabilities.minImageCount + 1;

    if (swap_chain_support.capabilities.maxImageCount > 0 &&
        image_count > swap_chain_support.capabilities.maxImageCount) {
        image_count = swap_chain_support.capabilities.maxImageCount;
    }

    // Define swap chain creation information
    VkSwapchainCreateInfoKHR create_info = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = surface,
        .minImageCount = image_count,
        .imageFormat = surface_format.format,
        .imageColorSpace = surface_format.colorSpace,
        .imageExtent = extent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .preTransform = swap_chain_support.capabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = present_mode,
        .clipped = VK_TRUE,
        .oldSwapchain = VK_NULL_HANDLE,
    };

    QueueFamilyIndices indices = get_queue_families(physical_device, surface);
    if (indices.graphics != indices.presentation) {
        uint32_t queue_family_indices[] = {
            *indices.graphics, *indices.presentation};
        create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        create_info.queueFamilyIndexCount = 2;
        create_info.pQueueFamilyIndices = queue_family_indices;
    } else {
        create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        create_info.queueFamilyIndexCount = 0;
        create_info.pQueueFamilyIndices = nullptr;
    }

    // Create swap chain
    if (vkCreateSwapchainKHR(
            device, &create_info, nullptr, &swap_chain.handle
        ) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create swap chain");
    }

    swap_chain.format = surface_format.format;
    swap_chain.extent = extent;

    // Get swap chain images
    vkGetSwapchainImagesKHR(device, swap_chain.handle, &image_count, nullptr);
    swap_chain.images.resize(image_count);
    vkGetSwapchainImagesKHR(
        device, swap_chain.handle, &image_count, swap_chain.images.data()
    );
}

void Vol::VulkanContext::create_image_views() {
    swap_chain.image_views.resize(swap_chain.images.size());

    VkComponentMapping components = {
        .r = VK_COMPONENT_SWIZZLE_IDENTITY,
        .g = VK_COMPONENT_SWIZZLE_IDENTITY,
        .b = VK_COMPONENT_SWIZZLE_IDENTITY,
        .a = VK_COMPONENT_SWIZZLE_IDENTITY,
    };

    VkImageSubresourceRange subresourceRange = {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = 1,
    };

    for (size_t i = 0; i < swap_chain.images.size(); i++) {
        VkImageViewCreateInfo create_info = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = swap_chain.images[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = swap_chain.format,
            .components = components,
            .subresourceRange = subresourceRange,
        };

        if (vkCreateImageView(
                device, &create_info, nullptr, &swap_chain.image_views[i]
            ) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create image view");
        }
    }
}

void Vol::VulkanContext::create_render_pass() {
    // Define color attachment
    VkAttachmentDescription color_attachment = {
        .format = swap_chain.format,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
    };

    VkAttachmentReference color_attachment_reference = {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };

    // Create color subpass
    VkSubpassDescription subpass = {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &color_attachment_reference,
    };

    // Create subpass dependancy
    VkSubpassDependency dependancy = {
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
    };

    // Define render pass creation information
    VkRenderPassCreateInfo render_pass_create_info = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &color_attachment,
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 1,
        .pDependencies = &dependancy,
    };

    if (vkCreateRenderPass(
            device, &render_pass_create_info, nullptr, &render_pass
        ) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create render pass");
    }
}

void Vol::VulkanContext::create_graphics_pipeline() {
    // Read SPIR-V files
    auto vert_code = read_binary_file(RES("shaders/basic_vert.spv"));
    auto frag_code = read_binary_file(RES("shaders/basic_frag.spv"));

    // Create shader modules
    VkShaderModule vert_module = create_shader_module(device, vert_code);
    VkShaderModule frag_module = create_shader_module(device, frag_code);

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
            device, &pipeline_layout_create_info, nullptr, &pipeline_layout
        ) != VK_SUCCESS) {
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
        .pColorBlendState = &color_blend_create_info,
        .pDynamicState = &dynamic_state_create_info,
        .layout = pipeline_layout,
        .renderPass = render_pass,
        .subpass = 0,
    };

    if (vkCreateGraphicsPipelines(
            device, VK_NULL_HANDLE, 1, &pipeline_create_info, nullptr,
            &graphics_pipeline
        ) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create graphics pipeline");
    }

    // Destoy shader modules
    vkDestroyShaderModule(device, vert_module, nullptr);
    vkDestroyShaderModule(device, frag_module, nullptr);
}

void Vol::VulkanContext::create_framebuffers() {
    swap_chain.frame_buffers.resize(swap_chain.image_views.size());

    for (size_t i = 0; i < swap_chain.image_views.size(); i++) {
        // Define framebuffer creation information
        VkFramebufferCreateInfo create_info = {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = render_pass,
            .attachmentCount = 1,
            .pAttachments = &swap_chain.image_views[i],
            .width = swap_chain.extent.width,
            .height = swap_chain.extent.height,
            .layers = 1,
        };

        // Create framebuffer
        if (vkCreateFramebuffer(
                device, &create_info, nullptr, &swap_chain.frame_buffers[i]
            ) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create framebuffer");
        }
    }
}

void Vol::VulkanContext::create_command_pool() {
    QueueFamilyIndices queue_familiy_indices =
        get_queue_families(physical_device, surface);

    // Define command pool creation information
    VkCommandPoolCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = *queue_familiy_indices.graphics,
    };

    // Create command pool
    if (vkCreateCommandPool(device, &create_info, nullptr, &command_pool) !=
        VK_SUCCESS) {
        throw std::runtime_error("Failed to create command pool");
    }
}

void Vol::VulkanContext::create_vertex_buffer() {
    // Define vertex buffer creation info
    VkBufferCreateInfo buffer_create_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = sizeof(Vertex) * vertices.size(),
        .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };

    // Create vertex buffer
    if (vkCreateBuffer(device, &buffer_create_info, nullptr, &vertex_buffer) !=
        VK_SUCCESS) {
        throw std::runtime_error("Failed to create vertex buffer");
    }

    // Get memory requirements
    VkMemoryRequirements mem_requirements;
    vkGetBufferMemoryRequirements(device, vertex_buffer, &mem_requirements);

    // Define memory allocation information
    VkMemoryAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = mem_requirements.size,
        .memoryTypeIndex = find_memory_type(
            physical_device, mem_requirements.memoryTypeBits,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        ),
    };

    // Allocate memory
    if (vkAllocateMemory(device, &alloc_info, nullptr, &vertex_buffer_memory) !=
        VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate vertex buffer memory");
    }

    // Bind memory
    vkBindBufferMemory(device, vertex_buffer, vertex_buffer_memory, 0);

    // Fill buffer
    void *data;
    vkMapMemory(
        device, vertex_buffer_memory, 0, buffer_create_info.size, 0, &data
    );
    memcpy(data, vertices.data(), static_cast<size_t>(buffer_create_info.size));
    vkUnmapMemory(device, vertex_buffer_memory);
}

void Vol::VulkanContext::allocate_command_buffers() {
    command_buffers.resize(MAX_FRAMES_IN_FLIGHT);

    // Define command buffer allocation information
    VkCommandBufferAllocateInfo allocation_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = command_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = MAX_FRAMES_IN_FLIGHT,
    };

    // Allocate command buffer
    if (vkAllocateCommandBuffers(
            device, &allocation_info, command_buffers.data()
        ) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate command buffer");
    }
}

void Vol::VulkanContext::create_sync_objects() {
    image_available_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
    render_finished_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
    in_flight_fences.resize(MAX_FRAMES_IN_FLIGHT);

    // Define semaphore creation info
    VkSemaphoreCreateInfo semaphore_create_info = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    };

    // Define fence creation info
    VkFenceCreateInfo fence_create_info = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT,
    };

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateSemaphore(
                device, &semaphore_create_info, nullptr,
                &image_available_semaphores[i]
            ) != VK_SUCCESS ||
            vkCreateSemaphore(
                device, &semaphore_create_info, nullptr,
                &render_finished_semaphores[i]
            ) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create semaphores");
        }

        if (vkCreateFence(
                device, &fence_create_info, nullptr, &in_flight_fences[i]
            ) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create fence");
        }
    }
}

void Vol::VulkanContext::record_command_buffer(
    VkCommandBuffer command_buffer,
    uint32_t image_index
) {
    // Define command buffer begin information
    VkCommandBufferBeginInfo begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = 0,
        .pInheritanceInfo = nullptr,
    };

    // Begin command buffer
    if (vkBeginCommandBuffer(command_buffer, &begin_info) != VK_SUCCESS) {
        throw std::runtime_error("Failed to begin command buffer");
    }

    // Define render pass begin info
    VkClearValue clear_color = {{{0.0f, 0.0f, 0.0f, 1.0f}}};

    VkRenderPassBeginInfo render_pass_begin_info = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = render_pass,
        .framebuffer = swap_chain.frame_buffers[image_index],
        .renderArea = {{0, 0}, swap_chain.extent},
        .clearValueCount = 1,
        .pClearValues = &clear_color,
    };

    vkCmdBeginRenderPass(
        command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE
    );

    vkCmdBindPipeline(
        command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline
    );

    // Set viewport
    VkViewport viewport = {
        .x = 0.0f,
        .y = 0.0f,
        .width = static_cast<float>(swap_chain.extent.width),
        .height = static_cast<float>(swap_chain.extent.height),
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };
    vkCmdSetViewport(command_buffer, 0, 1, &viewport);

    // Define scissor
    VkRect2D scissor = {
        .offset = {0, 0},
        .extent = swap_chain.extent,
    };
    vkCmdSetScissor(command_buffer, 0, 1, &scissor);

    // Bind buffers
    VkBuffer vertex_buffers[] = {vertex_buffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(command_buffer, 0, 1, vertex_buffers, offsets);

    // Draw
    vkCmdDraw(command_buffer, static_cast<uint32_t>(vertices.size()), 1, 0, 0);

    // End render pass
    vkCmdEndRenderPass(command_buffer);

    // End command buffer
    if (vkEndCommandBuffer(command_buffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to end command buffer");
    }
}

void Vol::VulkanContext::cleanup_swap_chain() {
    for (auto framebuffer : swap_chain.frame_buffers) {
        vkDestroyFramebuffer(device, framebuffer, nullptr);
    }

    for (auto image_view : swap_chain.image_views) {
        vkDestroyImageView(device, image_view, nullptr);
    }
    vkDestroySwapchainKHR(device, swap_chain.handle, nullptr);
}

bool validation_layers_supported() {
    // Query and retrieve available layers
    uint32_t available_layer_count;
    vkEnumerateInstanceLayerProperties(&available_layer_count, nullptr);

    std::vector<VkLayerProperties> available_layers(available_layer_count);
    vkEnumerateInstanceLayerProperties(
        &available_layer_count, available_layers.data()
    );

    // Compare requested validation layers with available layers
    for (const char *validation_layer_name : validation_layers) {
        bool layer_found = false;
        for (const auto &available_layer : available_layers) {
            if (strcmp(validation_layer_name, available_layer.layerName)) {
                layer_found = true;
                break;
            }
        }
        if (!layer_found) {
            return false;
        }
    }
    return true;
}

std::optional<unsigned int> get_physical_device_ranking(
    VkPhysicalDevice device,
    VkSurfaceKHR surface
) {
    // Check if device is suitable
    if (!is_physical_device_suitable(device, surface)) {
        return std::nullopt;
    }

    // Get device information
    VkPhysicalDeviceProperties device_props;
    VkPhysicalDeviceFeatures device_features;
    vkGetPhysicalDeviceProperties(device, &device_props);
    vkGetPhysicalDeviceFeatures(device, &device_features);

    // Rank device
    int score = 0;

    if (device_props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
        score += 1000;
    }

    score += device_props.limits.maxImageDimension2D;

    return score;
}

bool is_physical_device_suitable(
    VkPhysicalDevice device,
    VkSurfaceKHR surface
) {
    // Check device support required queue families
    if (!get_queue_families(device, surface).is_complete()) {
        return false;
    }

    // Check device extension support
    uint32_t extension_count;
    vkEnumerateDeviceExtensionProperties(
        device, nullptr, &extension_count, nullptr
    );
    std::vector<VkExtensionProperties> extensions_props(extension_count);
    vkEnumerateDeviceExtensionProperties(
        device, nullptr, &extension_count, extensions_props.data()
    );

    std::set<std::string> required_extensions(
        required_physical_device_extensions.begin(),
        required_physical_device_extensions.end()
    );

    for (const auto &extension_props : extensions_props) {
        required_extensions.erase(extension_props.extensionName);
    }

    if (!required_extensions.empty()) {
        return false;
    }

    // Check swap chain support
    SwapChainSupportDetails swap_chain_support =
        get_swap_chain_support(device, surface);
    if (swap_chain_support.formats.empty() ||
        swap_chain_support.present_modes.empty()) {
        return false;
    }

    return true;
}

QueueFamilyIndices get_queue_families(
    VkPhysicalDevice device,
    VkSurfaceKHR surface
) {
    // Query and retrieve queue information
    uint32_t queue_count;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_count, nullptr);

    std::vector<VkQueueFamilyProperties> queue_props(queue_count);
    vkGetPhysicalDeviceQueueFamilyProperties(
        device, &queue_count, queue_props.data()
    );

    // Scan queues
    QueueFamilyIndices queue_indices{};

    for (size_t i = 0; i < queue_count; i++) {
        // Check queue for graphics support
        if (!queue_indices.graphics &&
            (queue_props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
            queue_indices.graphics = i;
        }

        // Check queue for presentation support
        if (!queue_indices.presentation) {
            VkBool32 present_support;
            vkGetPhysicalDeviceSurfaceSupportKHR(
                device, i, surface, &present_support
            );
            if (present_support) {
                queue_indices.presentation = i;
            }
        }

        if (queue_indices.is_complete()) {
            break;
        }
    }

    if (!queue_indices.is_complete()) {
        throw std::runtime_error("Failed to find all required queues");
    }

    return queue_indices;
}

SwapChainSupportDetails get_swap_chain_support(
    VkPhysicalDevice device,
    VkSurfaceKHR surface
) {
    SwapChainSupportDetails support_details{};

    // Get capabilities
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
        device, surface, &support_details.capabilities
    );

    // Get supported formats
    uint32_t format_count;
    vkGetPhysicalDeviceSurfaceFormatsKHR(
        device, surface, &format_count, nullptr
    );

    support_details.formats.resize(format_count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(
        device, surface, &format_count, support_details.formats.data()
    );

    // Get present modes
    uint32_t present_mode_count;
    vkGetPhysicalDeviceSurfacePresentModesKHR(
        device, surface, &present_mode_count, nullptr
    );

    support_details.present_modes.resize(present_mode_count);
    vkGetPhysicalDeviceSurfacePresentModesKHR(
        device, surface, &present_mode_count,
        support_details.present_modes.data()
    );

    return support_details;
}

VkSurfaceFormatKHR select_swap_surface_format(
    const std::vector<VkSurfaceFormatKHR> &available_formats
) {
    for (const auto &available_format : available_formats) {
        if (available_format.format == VK_FORMAT_B8G8R8A8_SRGB &&
            available_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return available_format;
        }
    }
    return available_formats[0];
}

VkPresentModeKHR select_swap_present_mode(
    const std::vector<VkPresentModeKHR> &available_present_modes
) {
    for (const auto &available_present_mode : available_present_modes) {
        if (available_present_mode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return available_present_mode;
        }
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D select_swap_extent(
    SDL_Window *window,
    const VkSurfaceCapabilitiesKHR &capabilities
) {
    // Check if current extent is acceptable
    if (capabilities.currentExtent.width !=
        std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    }

    // Manually calculate extent
    int w, h;
    SDL_GetWindowSizeInPixels(window, &w, &h);

    VkExtent2D extent = {
        static_cast<uint32_t>(w),
        static_cast<uint32_t>(h),
    };

    extent.width = std::clamp(
        extent.width, capabilities.minImageExtent.width,
        capabilities.maxImageExtent.width
    );
    extent.height = std::clamp(
        extent.height, capabilities.minImageExtent.height,
        capabilities.maxImageExtent.height
    );

    return extent;
}

std::vector<char> read_binary_file(const std::string &filename) {
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
    const std::vector<char> &code
) {
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
    VkMemoryPropertyFlags properties
) {
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
