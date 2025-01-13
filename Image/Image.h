#pragma once

#include <vulkan/vulkan_core.h>

namespace graphics
{

struct CreateImageInfo
{
    uint32_t width;
    uint32_t height;
    uint32_t mip_levels;
    VkSampleCountFlagBits num_samples;
    VkFormat format;
    VkImageTiling tiling;
    VkImageUsageFlags usage;
    VkMemoryPropertyFlags properties;
};

class Image
{
public:
    void create(VkDevice device, const CreateImageInfo &image_info);
    
    void clean(VkDevice device);
private:
    VkImage image_;
    VkDeviceMemory image_memory_;
    VkImageView image_view_;
};

}


