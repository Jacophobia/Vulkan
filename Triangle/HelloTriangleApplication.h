// ReSharper disable CppClangTidyCppcoreguidelinesAvoidConstOrRefDataMembers
#pragma once

// We do not need to include vulkan ourselves as long as we have the
// following directive which tells glfw to do it
#define GLFW_INCLUDE_VULKAN

#include <string>
#include <vector>
#include <GLFW/glfw3.h>

#include "../Queue/QueueFamilyIndices.h"
#include "../SwapChain/SwapChainSupportDetails.h"

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

    const size_t max_frames_in_flight_ = 2;
    uint32_t current_frame_ = 0;
    
    GLFWwindow* window_;
    VkInstance instance_;
    VkDebugUtilsMessengerEXT debug_messenger_;
    VkPhysicalDevice physical_device_ = VK_NULL_HANDLE; // implicitly destroyed
    VkDevice device_;
    VkQueue graphics_queue_; // implicitly destroyed
    VkQueue present_queue_; // implicitly destroyed
    VkSurfaceKHR surface_;
    VkSwapchainKHR swap_chain_;
    std::vector<VkImage> swap_chain_images_;
    VkFormat swap_chain_image_format_;
    VkExtent2D swap_chain_extent_;
    std::vector<VkImageView> swap_chain_image_views_;
    VkRenderPass render_pass_;
    VkPipelineLayout pipeline_layout_;
    VkPipeline graphics_pipeline_;
    std::vector<VkFramebuffer> swap_chain_framebuffers_;
    VkCommandPool command_pool_;

    // per-flight
    std::vector<VkCommandBuffer> command_buffers_;
    std::vector<VkSemaphore> image_available_semaphores_;
    std::vector<VkSemaphore> render_finished_semaphores_;
    std::vector<VkFence> in_flight_fences_;
    
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
    SwapChainSupportDetails get_swap_chain_support_details(VkPhysicalDevice device);
    int rate_device(VkPhysicalDevice device);
    void select_physical_device();

    QueueFamilyIndices find_queue_families(VkPhysicalDevice device);
    void create_logical_device();

    void create_surface();

    VkSurfaceFormatKHR select_swap_surface_format(const std::vector<VkSurfaceFormatKHR>& available_formats);
    VkPresentModeKHR select_swap_present_mode(const std::vector<VkPresentModeKHR> &available_present_modes);
    VkExtent2D select_swap_extent(const VkSurfaceCapabilitiesKHR& capabilities);
    void create_swap_chain();

    void create_image_views();

    static std::vector<char> read_file(const std::string& filename);
    VkShaderModule create_shader_module(const std::vector<char>& code);

    void create_render_pass();
    
    void create_graphics_pipeline();

    void create_frame_buffers();

    void create_command_pool();

    void create_command_buffers();

    void create_sync_objects();

    void record_command_buffer(VkCommandBuffer command_buffer, uint32_t image_index);

    void main_loop();
    void draw_frame();
    void clean_up();
};
