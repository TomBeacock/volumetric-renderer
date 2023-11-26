#include "imgui_context.h"

#include "application.h"
#include "rendering/main_pass.h"
#include "rendering/offscreen_pass.h"
#include "rendering/util.h"
#include "rendering/vulkan_context.h"

#include <SDL3/SDL.h>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_vulkan.h>
#include <imgui.h>
#include <vulkan/vulkan.h>

#include <stdexcept>

Vol::UI::ImGuiContext::ImGuiContext(
    SDL_Window *window,
    Rendering::VulkanContext *vulkan_context)
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

Vol::UI::ImGuiContext::~ImGuiContext()
{
    // Destory descriptor pool
    vkDestroyDescriptorPool(
        vulkan_context->get_device(), descriptor_pool, nullptr);
    // Shutdown ImGui
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
}

void Vol::UI::ImGuiContext::process_event(SDL_Event *event)
{
    ImGui_ImplSDL3_ProcessEvent(event);
}

void Vol::UI::ImGuiContext::end_frame()
{
    ImGui::EndFrame();
}

void Vol::UI::ImGuiContext::recreate_viewport_texture(
    uint32_t width,
    uint32_t height)
{
    if (width == 0 || height == 0) {
        return;
    }

    vulkan_context->wait_till_idle();

    ImGui_ImplVulkan_RemoveTexture(descriptor);

    Vol::Rendering::OffscreenPass *const offscreen_pass =
        Application::main().get_vulkan_context().get_offscreen_pass();

    offscreen_pass->framebuffer_size_changed(width, height);

    descriptor = ImGui_ImplVulkan_AddTexture(
        offscreen_pass->get_sampler(), offscreen_pass->get_image_view(),
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

void Vol::UI::ImGuiContext::init_backends(SDL_Window *window)
{
    // Initialize ImGui for SDL3
    ImGui_ImplSDL3_InitForVulkan(window);

    // Initialize ImGui for Vulkan
    VkDescriptorPoolSize pool_sizes[] = {
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2},
    };

    VkDescriptorPoolCreateInfo pool_create_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
        .maxSets = 2,
        .poolSizeCount = std::size(pool_sizes),
        .pPoolSizes = pool_sizes,
    };

    if (vkCreateDescriptorPool(
            vulkan_context->get_device(), &pool_create_info, nullptr,
            &descriptor_pool) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create descriptor pool");
    }

    Vol::Rendering::SwapChainSupportDetails swapchain_details =
        Vol::Rendering::get_swap_chain_support(
            vulkan_context->get_physical_device(),
            vulkan_context->get_surface());

    ImGui_ImplVulkan_InitInfo init_info = {
        .Instance = vulkan_context->get_instance(),
        .PhysicalDevice = vulkan_context->get_physical_device(),
        .Device = vulkan_context->get_device(),
        .Queue = vulkan_context->get_graphics_queue(),
        .DescriptorPool = descriptor_pool,
        .MinImageCount = swapchain_details.capabilities.minImageCount,
        .ImageCount = static_cast<uint32_t>(
            vulkan_context->get_main_pass()->get_swap_chain().images.size()),
    };

    ImGui_ImplVulkan_Init(
        &init_info, vulkan_context->get_main_pass()->get_render_pass());

    // Upload fonts
    {
        VkCommandBuffer command_buffer = vulkan_context->begin_single_command();
        ImGui_ImplVulkan_CreateFontsTexture(command_buffer);
        vulkan_context->end_single_command(command_buffer);
        ImGui_ImplVulkan_DestroyFontUploadObjects();
    }

    // Create descriptor set
    Vol::Rendering::OffscreenPass *const offscreen_pass =
        Application::main().get_vulkan_context().get_offscreen_pass();

    descriptor = ImGui_ImplVulkan_AddTexture(
        offscreen_pass->get_sampler(), offscreen_pass->get_image_view(),
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}
