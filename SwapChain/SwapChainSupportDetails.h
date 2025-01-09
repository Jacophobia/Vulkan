#pragma once

#include <vector>
#include <vulkan/vulkan_core.h>

struct SwapChainSupportDetails
{
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> present_modes;

    [[nodiscard]] bool is_complete() const;
};
