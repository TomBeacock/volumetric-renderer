#include "util.h"

#include <stdexcept>

Vol::Rendering::QueueFamilyIndices Vol::Rendering::get_queue_families(
    VkPhysicalDevice device,
    VkSurfaceKHR surface)
{
    // Query and retrieve queue information
    uint32_t queue_count;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_count, nullptr);

    std::vector<VkQueueFamilyProperties> queue_props(queue_count);
    vkGetPhysicalDeviceQueueFamilyProperties(
        device, &queue_count, queue_props.data());

    // Scan queues
    Vol::Rendering::QueueFamilyIndices queue_indices{};

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
                device, i, surface, &present_support);
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

Vol::Rendering::SwapChainSupportDetails Vol::Rendering::get_swap_chain_support(
    VkPhysicalDevice device,
    VkSurfaceKHR surface)
{
    Vol::Rendering::SwapChainSupportDetails support_details{};

    // Get capabilities
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
        device, surface, &support_details.capabilities);

    // Get supported formats
    uint32_t format_count;
    vkGetPhysicalDeviceSurfaceFormatsKHR(
        device, surface, &format_count, nullptr);

    support_details.formats.resize(format_count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(
        device, surface, &format_count, support_details.formats.data());

    // Get present modes
    uint32_t present_mode_count;
    vkGetPhysicalDeviceSurfacePresentModesKHR(
        device, surface, &present_mode_count, nullptr);

    support_details.present_modes.resize(present_mode_count);
    vkGetPhysicalDeviceSurfacePresentModesKHR(
        device, surface, &present_mode_count,
        support_details.present_modes.data());

    return support_details;
}
