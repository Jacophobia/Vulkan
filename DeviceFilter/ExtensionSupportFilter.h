#pragma once

#include "DeviceFilter.h"

class ExtensionSupportFilter : public DeviceFilter
{
    bool is_valid(VkPhysicalDevice device) override;
};
