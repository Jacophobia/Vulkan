#pragma once

// We do not need to include vulkan ourselves as long as we have the
// following directive which tells glfw to do it
#define GLFW_INCLUDE_VULKAN

#include <vector>
#include <GLFW/glfw3.h>

#include "../Queue/QueueFamilyIndices.h"

class HelloTriangleApplication
{
public:
    HelloTriangleApplication();
    ~HelloTriangleApplication();
    
    void run();

private:
    // constants
    const int width_ = 800;
    const int height_ = 600;
    const char* title_ = "Vulkan";
    const std::vector<const char*> validation_layers_ =
    {
        "VK_LAYER_KHRONOS_validation",
    };
    const std::vector<const char*> device_extensions_ =
    {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    };
#ifdef NDEBUG
    const bool enable_validation_layers_ = false;
#else
    const bool enable_validation_layers_ = true;
#endif
    
    GLFWwindow* window_;
    VkInstance instance_;
    VkDebugUtilsMessengerEXT debug_messenger_;
    VkPhysicalDevice physical_device_ = VK_NULL_HANDLE; // implicitly destroyed
    VkDevice device_;
    VkQueue graphics_queue_; // implicitly destroyed
    VkQueue present_queue_; // implicitly destroyed
    VkSurfaceKHR surface_;
    
    void init_window();

    /* Vulkan Initialization */
    void init_vulkan();
    void create_instance();
    void populate_debug_messenger_create_info(VkDebugUtilsMessengerCreateInfoEXT &create_info);
    void set_up_debug_messenger();
    std::vector<const char*> get_required_extensions();
    bool are_extensions_supported(const std::vector<const char *> &glfw_extensions);
    bool are_validation_layers_supported();

    bool are_device_extensions_supported(const VkPhysicalDevice device);
    int rate_device(VkPhysicalDevice device);
    void select_physical_device();

    QueueFamilyIndices find_queue_families(VkPhysicalDevice device);
    void create_logical_device();

    void create_surface();
    
    void main_loop();
    void clean_up();
};
