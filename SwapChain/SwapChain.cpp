#include "SwapChain.h"

void SwapChain::clean(VkDevice device)
{
    // vkDestroyImageView(device, color_image_view_, nullptr);
    // vkDestroyImage(device, color_image_, nullptr);
    // vkFreeMemory(device, color_image_memory_, nullptr);
    //
    // vkDestroyImageView(device, depth_image_view_, nullptr);
    // vkDestroyImage(device, depth_image_, nullptr);
    // vkFreeMemory(device, depth_image_memory_, nullptr);
    //
    // for (const auto framebuffer : swap_chain_framebuffers_)
    // {
    //     vkDestroyFramebuffer(device, framebuffer, nullptr);
    // }
    //
    // for (const auto image_view : swap_chain_image_views_)
    // {
    //     vkDestroyImageView(device, image_view, nullptr);
    // }
    //
    // vkDestroySwapchainKHR(device, swap_chain_, nullptr);
}
