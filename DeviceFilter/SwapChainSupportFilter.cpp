#include "SwapChainSupportFilter.h"

bool SwapChainSupportFilter::is_valid(VkPhysicalDevice device)
{
    VkPhysicalDeviceFeatures device_features;
    vkGetPhysicalDeviceFeatures(device, &device_features);

    return device_features.geometryShader;
}
