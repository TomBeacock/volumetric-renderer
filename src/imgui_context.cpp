#include "imgui_context.h"

#include "vulkan_context.h"

#include <SDL3/SDL.h>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_vulkan.h>
#include <imgui.h>
#include <vulkan/vulkan.h>

#include <stdexcept>

Vol::ImGuiContext::ImGuiContext(
    SDL_Window *window,
    VulkanContext *vulkan_context)
    : vulkan_context(vulkan_context)
{
    // Create ImGui Context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.Fonts->AddFontFromFileTTF(RES_PATH "fonts/ARIAL.TTF", 14);
    io.Fonts->AddFontFromFileTTF(RES_PATH "fonts/ARIALBD.TTF", 16);
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // Init backends
    init_backends(window);
}

Vol::ImGuiContext::~ImGuiContext()
{
    // Destory descriptor pool
    vkDestroyDescriptorPool(vulkan_context->device, descriptor_pool, nullptr);
    // Shutdown ImGui
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
}

void Vol::ImGuiContext::process_event(SDL_Event *event)
{
    ImGui_ImplSDL3_ProcessEvent(event);
}

void Vol::ImGuiContext::render()
{
    VkCommandBuffer command_buffer =
        vulkan_context->command_buffers[vulkan_context->frame_index];

    ImGui::Render();
    ImDrawData *draw_data = ImGui::GetDrawData();
    ImGui_ImplVulkan_RenderDrawData(draw_data, command_buffer);
}

void Vol::ImGuiContext::end_frame()
{
    ImGui::EndFrame();
}

void Vol::ImGuiContext::init_backends(SDL_Window *window)
{
    // Initialize ImGui for SDL3
    ImGui_ImplSDL3_InitForVulkan(window);

    // Initialize ImGui for Vulkan
    VkDescriptorPoolSize pool_sizes[] = {
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1},
    };

    VkDescriptorPoolCreateInfo pool_create_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
        .maxSets = 1,
        .poolSizeCount = std::size(pool_sizes),
        .pPoolSizes = pool_sizes,
    };

    if (vkCreateDescriptorPool(
            vulkan_context->device, &pool_create_info, nullptr,
            &descriptor_pool) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create descriptor pool");
    }

    ImGui_ImplVulkan_InitInfo init_info = {
        .Instance = vulkan_context->instance,
        .PhysicalDevice = vulkan_context->physical_device,
        .Device = vulkan_context->device,
        .Queue = vulkan_context->present_queue,
        .DescriptorPool = descriptor_pool,
        .MinImageCount =
            static_cast<uint32_t>(vulkan_context->swap_chain.images.size() - 1),
        .ImageCount =
            static_cast<uint32_t>(vulkan_context->swap_chain.images.size()),
    };

    ImGui_ImplVulkan_Init(&init_info, vulkan_context->render_pass);

    // Upload fonts
    {
        // Get command pool and buffer
        VkCommandPool command_pool = vulkan_context->command_pool;
        VkCommandBuffer command_buffer =
            vulkan_context->command_buffers[vulkan_context->frame_index];

        // Reset command pool
        vkResetCommandPool(vulkan_context->device, command_pool, 0);

        // Define command buffer begin information
        VkCommandBufferBeginInfo begin_info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        };

        // Begin command buffer
        if (vkBeginCommandBuffer(command_buffer, &begin_info) != VK_SUCCESS) {
            throw std::runtime_error("Failed to begin command buffer");
        }

        // Create fonts
        ImGui_ImplVulkan_CreateFontsTexture(command_buffer);

        // End command buffer
        if (vkEndCommandBuffer(command_buffer) != VK_SUCCESS) {
            throw std::runtime_error("Failed to end command buffer");
        };

        // Submit command buffer
        VkSubmitInfo end_info = {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .commandBufferCount = 1,
            .pCommandBuffers = &command_buffer,
        };
        if (vkQueueSubmit(
                vulkan_context->present_queue, 1, &end_info, VK_NULL_HANDLE) !=
            VK_SUCCESS) {
            throw std::runtime_error("Failed to submit fonts command buffer");
        };

        // Wait till device idle
        vkDeviceWaitIdle(vulkan_context->device);

        ImGui_ImplVulkan_DestroyFontUploadObjects();
    }
}
