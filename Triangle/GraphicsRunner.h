﻿// ReSharper disable CppClangTidyCppcoreguidelinesAvoidConstOrRefDataMembers
#pragma once

// We do not need to include vulkan ourselves as long as we have the
// following directive which tells glfw to do it
#define GLFW_INCLUDE_VULKAN

#include <string>
#include <vector>
#include <GLFW/glfw3.h>

#include "../Queue/QueueFamilyIndices.h"
#include "../Rendering/Vertex.h"
#include "../SwapChain/SwapChainSupportDetails.h"

class GraphicsRunner
{
public:
    bool frame_buffer_resized = false;
    
    GraphicsRunner();
    ~GraphicsRunner();
    
    void run();

private:
    // constants
    const int width_ = 800;
    const int height_ = 600;
    const char* title_ = "Vulkan";

    const std::string model_path_ = "Models/viking_room.obj";
    const std::string texture_path_ = "Textures/viking_room.png";
    
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
    VkDescriptorSetLayout descriptor_set_layout_;
    VkPipelineLayout pipeline_layout_;
    VkPipeline graphics_pipeline_;
    std::vector<VkFramebuffer> swap_chain_framebuffers_;
    VkCommandPool command_pool_;
    VkDescriptorPool descriptor_pool_;
    // implicitly destroyed when pool is destroyed
    std::vector<VkDescriptorSet> descriptor_sets_; 
    
    std::vector<Vertex> vertices_;
    std::vector<uint32_t> indices_;
    // TODO: Combine Buffers 
    VkBuffer vertex_buffer_;
    VkDeviceMemory vertex_buffer_memory_;
    VkBuffer index_buffer_;
    VkDeviceMemory index_buffer_memory_;

    std::vector<VkBuffer> uniform_buffers_;
    std::vector<VkDeviceMemory> uniform_buffers_memories_;
    std::vector<void*> uniform_buffers_mapped_;

    // per-flight
    std::vector<VkCommandBuffer> command_buffers_;
    std::vector<VkSemaphore> image_available_semaphores_;
    std::vector<VkSemaphore> render_finished_semaphores_;
    std::vector<VkFence> in_flight_fences_;

    uint32_t mip_levels_;
    VkImage texture_image_;
    VkDeviceMemory texture_image_memory_;
    VkImageView texture_image_view_;
    VkSampler texture_sampler_;
    
    VkImage depth_image_;
    VkDeviceMemory depth_image_memory_;
    VkImageView depth_image_view_;

    static void frame_buffer_resize_callback(GLFWwindow* window, int width, int height);
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
    void recreate_swap_chain();
    void create_swap_chain();

    void create_image_views();

    static std::vector<char> read_file(const std::string& filename);
    VkShaderModule create_shader_module(const std::vector<char>& code);

    void create_render_pass();

    void create_descriptor_set_layout();
    
    void create_graphics_pipeline();

    void create_frame_buffers();

    void create_command_pools();
    VkFormat find_supported_format(const std::vector<VkFormat> &candidates, VkImageTiling tiling,
                                   VkFormatFeatureFlags features);
    VkFormat find_depth_format();
    bool has_stencil_component(VkFormat format);

    void create_depth_resources();

    void create_image(uint32_t width, uint32_t height, uint32_t mip_levels, VkFormat format, VkImageTiling tiling, VkImageUsageFlags
                      usage, VkMemoryPropertyFlags properties, VkImage &image, VkDeviceMemory &image_memory);
    void generate_mip_maps(VkImage image, VkFormat image_format, int32_t texture_width,
                           int32_t texture_height, uint32_t mip_levels);

    void create_texture_image();
    
    VkImageView create_image_view(VkImage image, VkFormat format, VkImageAspectFlags aspect_flags, uint32_t mip_levels);

    void create_texture_image_view();

    void create_texture_sampler();

    void load_model();

    uint32_t find_memory_type(uint32_t type_filter, VkMemoryPropertyFlags properties);

    void create_buffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer &buffer, VkDeviceMemory &buffer_memory);
    void transition_image_layout(VkImage image, VkFormat format, VkImageLayout old_layout,
                                 VkImageLayout new_layout, uint32_t mip_levels);
    void copy_buffer_to_image(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
    VkCommandBuffer begin_single_time_commands();
    void end_single_time_commands(VkCommandBuffer command_buffer);

    void copy_buffer(VkBuffer src_buffer, VkBuffer dst_buffer, VkDeviceSize size);

    void create_vertex_buffer();

    void create_index_buffer();

    void create_uniform_buffers();

    void create_descriptor_pool();
    
    void create_descriptor_sets();

    void create_command_buffers();

    void create_sync_objects();

    void record_command_buffer(VkCommandBuffer command_buffer, uint32_t image_index);

    void main_loop();
    
    void draw_frame();
    
    void update_uniform_buffer();
    
    void clean_up();

    void clean_up_swap_chain();
};
