#include "main_pass.h"

#include "rendering/offscreen_pass.h"
#include "rendering/util.h"
#include "rendering/vulkan_context.h"

#include <SDL3/SDL.h>
#include <backends/imgui_impl_vulkan.h>
#include <glm/glm.hpp>
#include <imgui.h>

#include <algorithm>
#include <array>
#include <fstream>
#include <optional>
#include <stdexcept>

VkSurfaceFormatKHR select_swap_surface_format(
    const std::vector<VkSurfaceFormatKHR> &available_formats);

VkPresentModeKHR select_swap_present_mode(
    const std::vector<VkPresentModeKHR> &available_present_modes);

VkExtent2D select_swap_extent(
    SDL_Window *window,
    const VkSurfaceCapabilitiesKHR &capabilities);

Vol::Rendering::MainPass::MainPass(VulkanContext *context) : context(context)
{
    create_swap_chain();
    create_image_views();
    create_render_pass();
    create_framebuffers();
    create_sync_objects();
    allocate_command_buffers();
}

Vol::Rendering::MainPass::~MainPass()
{
    destroy_swap_chain();
    vkDestroyRenderPass(context->get_device(), render_pass, nullptr);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroyFence(context->get_device(), in_flight_fences[i], nullptr);
        vkDestroySemaphore(
            context->get_device(), image_available_semaphores[i], nullptr);
        vkDestroySemaphore(
            context->get_device(), render_finished_semaphores[i], nullptr);
    }
}

void Vol::Rendering::MainPass::render()
{
    // Wait to start frame
    vkWaitForFences(
        context->get_device(), 1, &in_flight_fences[frame_index], VK_TRUE,
        UINT64_MAX);

    // Get image index from swap chain
    VkResult result = vkAcquireNextImageKHR(
        context->get_device(), swap_chain.handle, UINT64_MAX,
        image_available_semaphores[frame_index], VK_NULL_HANDLE, &image_index);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreate_swap_chain();
        return;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("Failed to acquire swap chain image");
    }

    // Image successfully accquired, reset fence and begin frame
    vkResetFences(context->get_device(), 1, &in_flight_fences[frame_index]);

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

    // Record offscreen pass
    context->get_offscreen_pass()->record(command_buffer, frame_index);
    // Record main pass
    record(command_buffer);

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
            context->get_graphics_queue(), 1, &submit_info,
            in_flight_fences[frame_index])) {
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

    result = vkQueuePresentKHR(context->get_present_queue(), &present_info);

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

void Vol::Rendering::MainPass::framebuffer_size_changed()
{
    framebuffer_size_dirty = true;
}

void Vol::Rendering::MainPass::create_swap_chain()
{
    SwapChainSupportDetails swap_chain_support = get_swap_chain_support(
        context->get_physical_device(), context->get_surface());

    // Select swap chain details
    VkSurfaceFormatKHR surface_format =
        select_swap_surface_format(swap_chain_support.formats);
    VkPresentModeKHR present_mode =
        select_swap_present_mode(swap_chain_support.present_modes);
    VkExtent2D extent = select_swap_extent(
        context->get_window(), swap_chain_support.capabilities);

    // Select swap chain image count
    uint32_t image_count = swap_chain_support.capabilities.minImageCount + 1;

    if (swap_chain_support.capabilities.maxImageCount > 0 &&
        image_count > swap_chain_support.capabilities.maxImageCount) {
        image_count = swap_chain_support.capabilities.maxImageCount;
    }

    // Define swap chain creation information
    VkSwapchainCreateInfoKHR create_info = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = context->get_surface(),
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

    QueueFamilyIndices indices = get_queue_families(
        context->get_physical_device(), context->get_surface());
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
            context->get_device(), &create_info, nullptr, &swap_chain.handle) !=
        VK_SUCCESS) {
        throw std::runtime_error("Failed to create swap chain");
    }

    swap_chain.format = surface_format.format;
    swap_chain.extent = extent;

    // Get swap chain images
    vkGetSwapchainImagesKHR(
        context->get_device(), swap_chain.handle, &image_count, nullptr);
    swap_chain.images.resize(image_count);
    vkGetSwapchainImagesKHR(
        context->get_device(), swap_chain.handle, &image_count,
        swap_chain.images.data());
}

void Vol::Rendering::MainPass::create_image_views()
{
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
                context->get_device(), &create_info, nullptr,
                &swap_chain.image_views[i]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create image view");
        }
    }
}

void Vol::Rendering::MainPass::create_render_pass()
{
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
            context->get_device(), &render_pass_create_info, nullptr,
            &render_pass) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create render pass");
    }
}

void Vol::Rendering::MainPass::create_framebuffers()
{
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
                context->get_device(), &create_info, nullptr,
                &swap_chain.frame_buffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create framebuffer");
        }
    }
}

void Vol::Rendering::MainPass::create_sync_objects()
{
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
                context->get_device(), &semaphore_create_info, nullptr,
                &image_available_semaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(
                context->get_device(), &semaphore_create_info, nullptr,
                &render_finished_semaphores[i]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create semaphores");
        }

        if (vkCreateFence(
                context->get_device(), &fence_create_info, nullptr,
                &in_flight_fences[i]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create fence");
        }
    }
}

void Vol::Rendering::MainPass::allocate_command_buffers()
{
    command_buffers.resize(MAX_FRAMES_IN_FLIGHT);

    // Define command buffer allocation information
    VkCommandBufferAllocateInfo allocation_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = context->get_command_pool(),
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = MAX_FRAMES_IN_FLIGHT,
    };

    // Allocate command buffer
    if (vkAllocateCommandBuffers(
            context->get_device(), &allocation_info, command_buffers.data()) !=
        VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate command buffer");
    }
}

void Vol::Rendering::MainPass::recreate_swap_chain()
{
    context->wait_till_idle();

    destroy_swap_chain();

    create_swap_chain();
    create_image_views();
    create_framebuffers();
}

void Vol::Rendering::MainPass::destroy_swap_chain()
{
    for (auto framebuffer : swap_chain.frame_buffers) {
        vkDestroyFramebuffer(context->get_device(), framebuffer, nullptr);
    }

    for (auto image_view : swap_chain.image_views) {
        vkDestroyImageView(context->get_device(), image_view, nullptr);
    }
    vkDestroySwapchainKHR(context->get_device(), swap_chain.handle, nullptr);
}

void Vol::Rendering::MainPass::record(VkCommandBuffer command_buffer)
{
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
        command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

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

    // Render ImGui
    ImGui::Render();
    ImDrawData *draw_data = ImGui::GetDrawData();
    ImGui_ImplVulkan_RenderDrawData(draw_data, command_buffer);

    // End render pass
    vkCmdEndRenderPass(command_buffer);
}

VkSurfaceFormatKHR select_swap_surface_format(
    const std::vector<VkSurfaceFormatKHR> &available_formats)
{
    for (const auto &available_format : available_formats) {
        if (available_format.format == VK_FORMAT_B8G8R8A8_SRGB &&
            available_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return available_format;
        }
    }
    return available_formats[0];
}

VkPresentModeKHR select_swap_present_mode(
    const std::vector<VkPresentModeKHR> &available_present_modes)
{
    return VK_PRESENT_MODE_FIFO_KHR;
    // return VK_PRESENT_MODE_MAILBOX_KHR;
}

VkExtent2D select_swap_extent(
    SDL_Window *window,
    const VkSurfaceCapabilitiesKHR &capabilities)
{
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
        capabilities.maxImageExtent.width);
    extent.height = std::clamp(
        extent.height, capabilities.minImageExtent.height,
        capabilities.maxImageExtent.height);

    return extent;
}
