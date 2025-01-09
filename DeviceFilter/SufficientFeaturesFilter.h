#pragma once

#include "DeviceFilter.h"

class SufficientFeaturesFilter : public DeviceFilter
{
    bool is_valid(VkPhysicalDevice device) override;
};
