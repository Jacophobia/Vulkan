#include "Image.h"

#include <stdexcept>

void graphics::Image::create(VkDevice device, const CreateImageInfo &image_info)
{
    VkImageCreateInfo image_create_info{};
    image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_create_info.imageType = VK_IMAGE_TYPE_2D;
    image_create_info.extent.width = image_info.width;
    image_create_info.extent.height = image_info.height;
    image_create_info.extent.depth = 1;
    image_create_info.mipLevels = image_info.mip_levels;
    image_create_info.arrayLayers = 1;
    image_create_info.format = image_info.format;
    image_create_info.tiling = image_info.tiling;
    image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_create_info.usage = image_info.usage;
    image_create_info.samples = image_info.num_samples;
    image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(device, &image_create_info, nullptr, &image_) != VK_SUCCESS)
    {
        throw std::runtime_error("Error: unable to create image.");
    }

    VkMemoryRequirements memory_requirements;
    vkGetImageMemoryRequirements(device, image_, &memory_requirements);

    VkMemoryAllocateInfo allocate_info{};
    allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocate_info.allocationSize = memory_requirements.size;
    // allocate_info.memoryTypeIndex = find_memory_type(memory_requirements.memoryTypeBits, image_info.properties);

    if (vkAllocateMemory(device, &allocate_info, nullptr, &image_memory_) != VK_SUCCESS)
    {
        throw std::runtime_error("Error: unable to allocate image memory.");
    }

    vkBindImageMemory(device, image_, image_memory_, 0);

    
}

void graphics::Image::clean(VkDevice device)
{
    vkDestroyImageView(device, image_view_, nullptr);
    vkDestroyImage(device, image_, nullptr);
    vkFreeMemory(device, image_memory_, nullptr);
}
