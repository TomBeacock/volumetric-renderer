#include "vulkan_context.h"

#include "rendering/main_pass.h"
#include "rendering/offscreen_pass.h"
#include "rendering/util.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
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

const std::vector<const char *> required_physical_device_extensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
};

bool validation_layers_supported();

std::optional<unsigned int> get_physical_device_ranking(
    VkPhysicalDevice device,
    VkSurfaceKHR surface);

bool is_physical_device_suitable(VkPhysicalDevice device, VkSurfaceKHR surface);

Vol::Rendering::VulkanContext::VulkanContext(SDL_Window *window)
    : window(window)
{
    create_instance();
    create_surface();
    select_physical_device();
    create_device();
    create_command_pool();
    main_pass = new MainPass(this);
    offscreen_pass = new OffscreenPass(this, 100, 100);
}

Vol::Rendering::VulkanContext::~VulkanContext()
{
    delete offscreen_pass;
    delete main_pass;
    vkDestroyCommandPool(device, command_pool, nullptr);
    vkDestroyDevice(device, nullptr);
    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);
}

void Vol::Rendering::VulkanContext::render()
{
    main_pass->render();
}

void Vol::Rendering::VulkanContext::wait_till_idle()
{
    vkDeviceWaitIdle(device);
}

void Vol::Rendering::VulkanContext::create_instance()
{
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

void Vol::Rendering::VulkanContext::create_surface()
{
    if (!SDL_Vulkan_CreateSurface(window, instance, &surface)) {
        throw std::runtime_error("Failed to create surface");
    }
}

void Vol::Rendering::VulkanContext::select_physical_device()
{
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

void Vol::Rendering::VulkanContext::create_device()
{
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
            physical_device, &device_create_info, nullptr, &device) !=
        VK_SUCCESS) {
        throw std::runtime_error("Failed to create logical device");
    }

    // Get queues
    vkGetDeviceQueue(device, *queue_indices.graphics, 0, &graphics_queue);
    vkGetDeviceQueue(device, *queue_indices.presentation, 0, &present_queue);
}

void Vol::Rendering::VulkanContext::create_command_pool()
{
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

bool validation_layers_supported()
{
    // Query and retrieve available layers
    uint32_t available_layer_count;
    vkEnumerateInstanceLayerProperties(&available_layer_count, nullptr);

    std::vector<VkLayerProperties> available_layers(available_layer_count);
    vkEnumerateInstanceLayerProperties(
        &available_layer_count, available_layers.data());

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
    VkSurfaceKHR surface)
{
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

bool is_physical_device_suitable(VkPhysicalDevice device, VkSurfaceKHR surface)
{
    // Check device support required queue families
    if (!Vol::Rendering::get_queue_families(device, surface).is_complete()) {
        return false;
    }

    // Check device extension support
    uint32_t extension_count;
    vkEnumerateDeviceExtensionProperties(
        device, nullptr, &extension_count, nullptr);
    std::vector<VkExtensionProperties> extensions_props(extension_count);
    vkEnumerateDeviceExtensionProperties(
        device, nullptr, &extension_count, extensions_props.data());

    std::set<std::string> required_extensions(
        required_physical_device_extensions.begin(),
        required_physical_device_extensions.end());

    for (const auto &extension_props : extensions_props) {
        required_extensions.erase(extension_props.extensionName);
    }

    if (!required_extensions.empty()) {
        return false;
    }

    // Check swap chain support
    Vol::Rendering::SwapChainSupportDetails swap_chain_support =
        Vol::Rendering::get_swap_chain_support(device, surface);
    if (swap_chain_support.formats.empty() ||
        swap_chain_support.present_modes.empty()) {
        return false;
    }

    return true;
}
