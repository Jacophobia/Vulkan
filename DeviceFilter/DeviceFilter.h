#pragma once

#include <vector>
#include <vulkan/vulkan_core.h>

class DeviceFilter
{
public:
    void filter(std::vector<VkPhysicalDevice>& devices)
    {
        size_t new_size = devices.size();
        
        for (size_t i = 0; i < new_size;)
        {
            if (!is_valid(devices[i]))
            {
                std::swap(devices[i], devices[new_size - 1]);
                --new_size;
            }
            else
            {
                ++i;    
            }
        }

        devices.resize(new_size);
    }
private:
    virtual bool is_valid(VkPhysicalDevice device) = 0;
};
