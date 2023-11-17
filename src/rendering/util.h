#pragma once

#include <vulkan/vulkan.h>

#include <optional>
#include <vector>

namespace Vol::Rendering
{
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

QueueFamilyIndices get_queue_families(
    VkPhysicalDevice device,
    VkSurfaceKHR surface);

SwapChainSupportDetails get_swap_chain_support(
    VkPhysicalDevice device,
    VkSurfaceKHR surface);
}  // namespace Vol::Rendering