#pragma once

#include "DeviceFilter.h"

class SwapChainSupportFilter : public DeviceFilter
{
    bool is_valid(VkPhysicalDevice device) override;
};
