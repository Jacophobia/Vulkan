// ReSharper disable CppMemberFunctionMayBeConst
// ReSharper disable CppUseStructuredBinding
// ReSharper disable CppTooWideScopeInitStatement
#define VMA_IMPLEMENTATION

#include "GraphicsRunner.h"

#include <algorithm>
#include <stdexcept>
#include <vector>
#include <format>
#include <fstream>
#include <set>
#include <chrono>
#include <ranges>
#include <stb_image.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <unordered_map>
#include <vk_mem_alloc.h>

#include "../Logging/Logging.h"
#include "../Rendering/UniformBufferObject.h"
#include "../Rendering/Vertex.h"
#include "../Models/ModelLoading.h"

GraphicsRunner::GraphicsRunner(Camera* camera) :
    window_(nullptr), instance_(), debug_messenger_(), device_(),
    graphics_queue_(), present_queue_(), surface_(), swap_chain_(),
    swap_chain_image_format_(), swap_chain_extent_(), render_pass_(),
    pipeline_layout_(), graphics_pipeline_(), command_pool_(),
    camera_(camera) {}

GraphicsRunner::~GraphicsRunner() = default;

void GraphicsRunner::init()
{
    init_window();
    init_vulkan();
}

void GraphicsRunner::update()
{
    draw_frame();
}

uint32_t GraphicsRunner::register_resource(const ResourceInfo &info)
{
    RenderableResource resource;
    resource.id = nextResourceId_++;
    resource.model = info.model;

    // Load model (with caching)
    if (vertex_cache_.contains(info.model_path) && index_cache_.contains(info.model_path))
    {
        resource.vertices = vertex_cache_[info.model_path];
        resource.indices = index_cache_[info.model_path];
    }
    else
    {
        model_loading::load_model(resource.vertices, resource.indices, info.model_path);
        vertex_cache_[info.model_path] = resource.vertices;
        index_cache_[info.model_path] = resource.indices;
    }

    logging::info(std::format("Vertices' size: {}, Indices' size: {}",
                               resource.vertices.size(), resource.indices.size()));

    assert(!resource.vertices.empty());
    assert(!resource.indices.empty());

    // --- Create vertex buffer using a staging buffer with VMA ---
    const VkDeviceSize vertex_buffer_size = sizeof(Vertex) * resource.vertices.size();

    // Create staging buffer for vertices
    VkBuffer staging_vertex_buffer;
    VmaAllocation staging_vertex_allocation;
    create_buffer(vertex_buffer_size,
                  VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                  staging_vertex_buffer,
                  staging_vertex_allocation);

    void* data;
    vmaMapMemory(allocator_, staging_vertex_allocation, &data);
    memcpy(data, resource.vertices.data(), static_cast<size_t>(vertex_buffer_size));
    vmaUnmapMemory(allocator_, staging_vertex_allocation);

    // Create the actual vertex buffer on GPU memory
    create_buffer(vertex_buffer_size,
                  VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                  resource.vertexBuffer,
                  resource.vertexBufferAllocation);

    copy_buffer(staging_vertex_buffer, resource.vertexBuffer, vertex_buffer_size);

    // Destroy the staging vertex buffer using VMA
    vmaDestroyBuffer(allocator_, staging_vertex_buffer, staging_vertex_allocation);

    // --- Create index buffer using a staging buffer with VMA ---
    const VkDeviceSize index_buffer_size = sizeof(uint32_t) * resource.indices.size();

    VkBuffer staging_index_buffer;
    VmaAllocation staging_index_allocation;
    create_buffer(index_buffer_size,
                  VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                  staging_index_buffer,
                  staging_index_allocation);

    vmaMapMemory(allocator_, staging_index_allocation, &data);
    memcpy(data, resource.indices.data(), static_cast<size_t>(index_buffer_size));
    vmaUnmapMemory(allocator_, staging_index_allocation);

    create_buffer(index_buffer_size,
                  VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                  resource.indexBuffer,
                  resource.indexBufferAllocation);

    copy_buffer(staging_index_buffer, resource.indexBuffer, index_buffer_size);
    vmaDestroyBuffer(allocator_, staging_index_buffer, staging_index_allocation);

    // --- Create texture image, image view, and sampler ---
    // (Assuming create_texture_image has been updated to use VMA internally)
    create_texture_image(info.texture_path, resource);
    create_texture_image_view(resource);
    create_texture_sampler(resource);

    // Allocate a descriptor set for this resource’s texture.
    VkDescriptorSetAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorPool = descriptor_pool_;
    alloc_info.descriptorSetCount = 1;
    alloc_info.pSetLayouts = &texture_descriptor_set_layout_;
    if (vkAllocateDescriptorSets(device_, &alloc_info, &resource.texture_descriptor_set) != VK_SUCCESS)
    {
        throw std::runtime_error("Error: unable to allocate texture descriptor set for resource");
    }
    
    // Update the texture descriptor set with the resource’s texture info.
    VkDescriptorImageInfo image_info{};
    image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    image_info.imageView = resource.texture_image_view;
    image_info.sampler = resource.texture_sampler;
    
    VkWriteDescriptorSet descriptor_write{};
    descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptor_write.dstSet = resource.texture_descriptor_set;
    descriptor_write.dstBinding = 0;
    descriptor_write.dstArrayElement = 0;
    descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptor_write.descriptorCount = 1;
    descriptor_write.pImageInfo = &image_info;
    
    vkUpdateDescriptorSets(device_, 1, &descriptor_write, 0, nullptr);

    resources_[resource.id] = resource;
    return resource.id;
}

void GraphicsRunner::update_resource(const uint32_t resource_id, const glm::mat4 &new_ubo)
{
    if (resources_.contains(resource_id))
    {
        resources_[resource_id].model = new_ubo;
    }
    else
    {
        throw std::runtime_error("Error: Resource ID not found during update.");
    }
}

void GraphicsRunner::unregister_resource(uint32_t resource_id)
{
    if (resources_.contains(resource_id))
    {
        vkDestroySampler(device_, resources_[resource_id].texture_sampler, nullptr);
        vkDestroyImageView(device_, resources_[resource_id].texture_image_view, nullptr);
        vmaDestroyImage(allocator_, resources_[resource_id].texture_image, resources_[resource_id].textureImageAllocation);

        vmaDestroyBuffer(allocator_, resources_[resource_id].vertexBuffer, resources_[resource_id].vertexBufferAllocation);
        vmaDestroyBuffer(allocator_, resources_[resource_id].indexBuffer, resources_[resource_id].indexBufferAllocation);
        resources_.erase(resource_id);
    }
    else
    {
        throw std::runtime_error("Error: Resource ID not found during unregister.");
    }
}

void GraphicsRunner::frame_buffer_resize_callback(GLFWwindow *window, int width, int height)
{
    const auto app = static_cast<GraphicsRunner*>(glfwGetWindowUserPointer(window));
    app->frame_buffer_resized = true;
    
    // this is a workaround to render while resizing
    if (width > 0 && height > 0)
    {
        // recreates the frame buffers
        app->draw_frame();
        // actually draws the frame
        app->draw_frame();
    }
}

void GraphicsRunner::init_window()
{
    // initialize a window
    glfwInit();
    // don't load opengl
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    // disable window resizing
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    
    window_ = glfwCreateWindow(width_, height_, title_, nullptr, nullptr);
    glfwSetWindowUserPointer(window_, this);
    glfwSetFramebufferSizeCallback(window_, frame_buffer_resize_callback);
}

void GraphicsRunner::init_vulkan()
{
    create_instance();
    set_up_debug_messenger();
    create_surface();
    select_physical_device();
    create_logical_device();
    create_vma_allocator();
    create_swap_chain();
    create_image_views();
    create_render_pass();
    create_global_descriptor_set_layout();
    create_texture_descriptor_set_layout();
    create_graphics_pipeline();
    create_command_pools();
    create_color_resources();
    create_depth_resources();
    create_frame_buffers();
    create_uniform_buffers();
    create_descriptor_pool();
    create_descriptor_sets();
    create_command_buffers();
    create_sync_objects();
}

void GraphicsRunner::create_instance()
{
    if (enable_validation_layers_ && !are_validation_layers_supported())
    {
        throw std::runtime_error("Error: validation layers requested, but not available.");
    }
    
    // create a vulkan instance
    VkApplicationInfo app_info{};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "Hello Triangle";
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "No Engine";
    app_info.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &app_info;

    auto glfw_extensions = get_required_extensions();

    create_info.enabledExtensionCount = static_cast<uint32_t>(glfw_extensions.size());
    create_info.ppEnabledExtensionNames = glfw_extensions.data();

    // check for extension support
    if (!are_extensions_supported(glfw_extensions))
    {
        throw std::runtime_error("Error: extension requested, but not available.");
    }

    VkDebugUtilsMessengerCreateInfoEXT debug_create_info{};
    if (enable_validation_layers_)
    {
        create_info.enabledLayerCount = static_cast<uint32_t>(validation_layers_.size());
        create_info.ppEnabledLayerNames = validation_layers_.data();

        populate_debug_messenger_create_info(debug_create_info);
        create_info.pNext = &debug_create_info;
    }
    else
    {
        create_info.enabledLayerCount = 0;

        create_info.pNext = nullptr;
    }

    if (vkCreateInstance(&create_info, nullptr, &instance_) != VK_SUCCESS)
    {
        throw std::runtime_error("Error: failed to create vulkan instance.");
    }
}

VkResult create_debug_utils_messenger(
    const VkInstance instance,
    const VkDebugUtilsMessengerCreateInfoEXT* p_create_info,
    const VkAllocationCallbacks* p_allocator,
    VkDebugUtilsMessengerEXT* p_debug_messenger)
{
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");

    if (func != nullptr)
    {
        return func(instance, p_create_info, p_allocator, p_debug_messenger);
    }

    return VK_ERROR_EXTENSION_NOT_PRESENT;
}

void destroy_debug_utils_messenger(
    const VkInstance instance,
    const VkDebugUtilsMessengerEXT debug_messenger,
    const VkAllocationCallbacks* p_allocator)
{
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    
    if (func != nullptr)
    {
        func(instance, debug_messenger, p_allocator);
    }
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    VkDebugUtilsMessageTypeFlagsEXT message_type,
    const VkDebugUtilsMessengerCallbackDataEXT* p_callback_data,
    void* p_user_data)
{
    logging::info(std::format("validation layer: {}", p_callback_data->pMessage));

    return VK_FALSE;
}

void GraphicsRunner::populate_debug_messenger_create_info(VkDebugUtilsMessengerCreateInfoEXT& create_info)
{
    create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
        | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
        | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
        | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
        | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    create_info.pfnUserCallback = debug_callback;
    create_info.pUserData = nullptr;
}

void GraphicsRunner::set_up_debug_messenger()
{
    if (!enable_validation_layers_)
    {
        return;
    }

    VkDebugUtilsMessengerCreateInfoEXT create_info;
    populate_debug_messenger_create_info(create_info);

    if (create_debug_utils_messenger(instance_, &create_info, nullptr, &debug_messenger_) != VK_SUCCESS)
    {
        throw std::runtime_error("Error: Failed to set up debug messenger");
    }
}

std::vector<const char *> GraphicsRunner::get_required_extensions()
{
    uint32_t glfw_extension_count = 0;

    const char **glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

    std::vector extensions(glfw_extensions, glfw_extensions + glfw_extension_count);

    if (enable_validation_layers_)
    {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
}

bool GraphicsRunner::are_extensions_supported(const std::vector<const char*> &glfw_extensions)
{
    uint32_t extension_count = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);

    std::vector<VkExtensionProperties> extensions(extension_count);
    
    vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, extensions.data());

    for (const auto &glfw_extension : glfw_extensions)
    {
        bool found = false;
        
        for (uint32_t extension_index = 0; extension_index < extension_count; extension_index++)
        {
            found = found || strcmp(extensions[extension_index].extensionName, glfw_extension) == 0;
        }

        if (!found)
        {
            logging::warning(std::format("Extension not found - {}", glfw_extension));
            return false;
        }
    }
    
    return true;
}

bool GraphicsRunner::are_validation_layers_supported()
{
    uint32_t layer_count;
    vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

    std::vector<VkLayerProperties> available_layers(layer_count);
    vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());

    for (const char* layer_name : validation_layers_) {
        bool layer_found = false;

        for (const auto& layer_properties : available_layers) {
            logging::info(std::format("Layer name - {}", layer_properties.layerName));
            if (strcmp(layer_name, layer_properties.layerName) == 0) {
                layer_found = true;
                break;
            }
        }

        if (!layer_found) {
            logging::warning(std::format("Validation layer not found - {}", layer_name));
            return false;
        }
    }

    return true;
}

bool GraphicsRunner::are_device_extensions_supported(const VkPhysicalDevice device)
{
    uint32_t extension_count = 0;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, nullptr);

    std::vector<VkExtensionProperties> available_extensions(extension_count);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, available_extensions.data());

    std::set<std::string> required_extensions(device_extensions_.begin(), device_extensions_.end());

    for (const auto& extension : available_extensions)
    {
        required_extensions.erase(extension.extensionName);
    }

    return required_extensions.empty();
}

SwapChainSupportDetails GraphicsRunner::get_swap_chain_support_details(VkPhysicalDevice device)
{
    SwapChainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface_, &details.capabilities);

    uint32_t format_count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface_, &format_count, nullptr);

    if (format_count != 0)
    {
        details.formats.resize(format_count);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface_, &format_count, details.formats.data());
    }

    uint32_t present_mode_count = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface_, &present_mode_count, nullptr);

    if (present_mode_count != 0)
    {
        details.present_modes.resize(present_mode_count);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface_, &present_mode_count, details.present_modes.data());
    }

    return details;
}

int GraphicsRunner::rate_device(const VkPhysicalDevice device)
{
    VkPhysicalDeviceProperties device_properties;
    vkGetPhysicalDeviceProperties(device, &device_properties);

    VkPhysicalDeviceFeatures device_features;
    vkGetPhysicalDeviceFeatures(device, &device_features);

    if (!device_features.samplerAnisotropy)
    {
        return 0;
    }

    if (!are_device_extensions_supported(device))
    {
        return 0;
    }
    
    auto swap_chain_details = get_swap_chain_support_details(device);
    if (!swap_chain_details.is_complete())
    {
        return 0;
    }

    if (!device_features.geometryShader)
    {
        return 0;
    }

    if (auto indices = find_queue_families(device); !indices.is_complete())
    {
        return 0;
    }

    int score = 0;
    
    if (device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
    {
        score += 1000;
    }

    score += device_properties.limits.maxImageDimension2D;

    return score;
}

VkSampleCountFlagBits GraphicsRunner::get_max_usable_sample_count()
{
    VkPhysicalDeviceProperties physical_device_properties;
    vkGetPhysicalDeviceProperties(physical_device_, &physical_device_properties);

    VkSampleCountFlags counts = physical_device_properties.limits.framebufferColorSampleCounts
        & physical_device_properties.limits.framebufferDepthSampleCounts;

    std::array possible_counts = {
        VK_SAMPLE_COUNT_64_BIT,
        VK_SAMPLE_COUNT_32_BIT,
        VK_SAMPLE_COUNT_16_BIT,
        VK_SAMPLE_COUNT_8_BIT,
        VK_SAMPLE_COUNT_4_BIT,
        VK_SAMPLE_COUNT_2_BIT,
    };

    for (const auto possible_count : possible_counts)
    {
        if (counts & possible_count)
        {
            return possible_count;
        }
    }
    
    return VK_SAMPLE_COUNT_1_BIT;
}

void GraphicsRunner::select_physical_device()
{
    uint32_t device_count = 0;
    vkEnumeratePhysicalDevices(instance_, &device_count, nullptr);

    if (device_count == 0)
    {
        throw std::runtime_error("Error: no GPUs found with Vulkan support.");
    }

    std::vector<VkPhysicalDevice> devices(device_count);
    vkEnumeratePhysicalDevices(instance_, &device_count, devices.data());

    int highest_rating = 0;
    for (const auto& device : devices)
    {
        const auto rating = rate_device(device);
        
        if (rating > highest_rating)
        {
            physical_device_ = device;
            highest_rating = rating;
        }
    }

    msaa_samples_ = get_max_usable_sample_count();

    if (highest_rating == 0 || physical_device_ == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Error: no suitable GPUs found.");
    }
}

QueueFamilyIndices GraphicsRunner::find_queue_families(VkPhysicalDevice device)
{
    QueueFamilyIndices indices;

    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);

    std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families.data());
    
    int i = 0;
    for (const auto& queue_family : queue_families)
    {
        bool graphics_support = false;
        if (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            graphics_support = true;
        }

        VkBool32 present_support = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface_, &present_support);
        
        // if it supports graphics and present, set it to that
        if (graphics_support && present_support)
        {
            indices.present_family = i;
            indices.graphics_family = i;
        }

        // set graphics family (as long as it hasn't already been set)
        if (graphics_support && !indices.graphics_family.has_value())
        {
            indices.graphics_family = i;
        }

        // set present family (as long as it hasn't already been set)
        if (present_support && !indices.present_family.has_value())
        {
            indices.present_family = i;
        }
        
        i++;
    }

    return indices;
}

void GraphicsRunner::create_logical_device()
{
    const auto indices = find_queue_families(physical_device_);
    
    if (!indices.graphics_family.has_value() || !indices.present_family.has_value())
    {
        throw std::runtime_error("Error: queue family not found");
    }

    std::set queue_families =
    {
        indices.graphics_family.value(),
        indices.present_family.value(),
    };

    std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
    for (auto queue_family : queue_families)
    {
        VkDeviceQueueCreateInfo queue_create_info{};
        queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_create_info.queueFamilyIndex = queue_family;
        queue_create_info.queueCount = 1;

        float queue_priority = 1.0f;
        queue_create_info.pQueuePriorities = &queue_priority;
        queue_create_infos.push_back(queue_create_info);
    }

    VkPhysicalDeviceFeatures device_features{};
    device_features.samplerAnisotropy = VK_TRUE;
    device_features.sampleRateShading = VK_TRUE;

    VkDeviceCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    create_info.queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size());
    create_info.pQueueCreateInfos = queue_create_infos.data();
    create_info.pEnabledFeatures = &device_features;
    create_info.enabledExtensionCount = static_cast<uint32_t>(device_extensions_.size());
    create_info.ppEnabledExtensionNames = device_extensions_.data();

    if (enable_validation_layers_)
    {
        create_info.enabledLayerCount = static_cast<uint32_t>(validation_layers_.size());
        create_info.ppEnabledLayerNames = validation_layers_.data();
    }
    else
    {
        create_info.enabledLayerCount = 0;
    }

    if (vkCreateDevice(physical_device_, &create_info, nullptr, &device_) != VK_SUCCESS)
    {
        throw std::runtime_error("Error: unable to create logical device.");
    }

    vkGetDeviceQueue(device_, indices.graphics_family.value(), 0, &graphics_queue_);
    vkGetDeviceQueue(device_, indices.present_family.value(), 0, &present_queue_);
}

void GraphicsRunner::create_vma_allocator()
{
    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.physicalDevice = physical_device_;
    allocatorInfo.device = device_;
    allocatorInfo.instance = instance_;
    allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_0;
    if (vmaCreateAllocator(&allocatorInfo, &allocator_) != VK_SUCCESS)
    {
        throw std::runtime_error("Error: failed to create VMA allocator.");
    }
}

void GraphicsRunner::create_surface()
{
    if (glfwCreateWindowSurface(instance_, window_, nullptr, &surface_) != VK_SUCCESS)
    {
        throw std::runtime_error("Error: unable to create surface.");
    }
}

VkSurfaceFormatKHR GraphicsRunner::select_swap_surface_format(const std::vector<VkSurfaceFormatKHR> &available_formats)
{
    if (available_formats.empty())
    {
        throw std::runtime_error("Error: no available surface formats.");
    }
    
    for (const auto& available_format : available_formats)
    {
        if (available_format.format == VK_FORMAT_B8G8R8A8_SRGB && available_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            return available_format;
        }
    }

    return available_formats[0];
}

VkPresentModeKHR GraphicsRunner::select_swap_present_mode(const std::vector<VkPresentModeKHR>& available_present_modes)
{
    for (const auto& available_present_mode : available_present_modes)
    {
        // TODO: switch to VK_PRESENT_MODE_MAILBOX_KHR if it gets more intensive to run. For now FIFO is good for not overworking the machine
        if (available_present_mode == VK_PRESENT_MODE_FIFO_KHR)
        {
            return available_present_mode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D GraphicsRunner::select_swap_extent(const VkSurfaceCapabilitiesKHR &capabilities)
{
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
    {
        return capabilities.currentExtent;
    }

    int width, height;

    glfwGetFramebufferSize(window_, &width, &height);

    VkExtent2D actual_extent =
    {
        static_cast<uint32_t>(width),
        static_cast<uint32_t>(height),
    };

    actual_extent.width = std::clamp(actual_extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    actual_extent.height = std::clamp(actual_extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

    return actual_extent;
}

void GraphicsRunner::recreate_swap_chain()
{
    int width = 0, height = 0;
    glfwGetFramebufferSize(window_, &width, &height);
    while (width == 0 && height == 0)
    {
        if (glfwWindowShouldClose(window_))
        {
            return;
        }
        
        glfwGetFramebufferSize(window_, &width, &height);
        glfwWaitEvents();
    }
    
    vkDeviceWaitIdle(device_);

    clean_up_swap_chain();
    
    create_swap_chain();
    create_image_views();
    create_color_resources();
    create_depth_resources();
    create_frame_buffers();
}

void GraphicsRunner::create_swap_chain()
{
    const auto swap_chain_details = get_swap_chain_support_details(physical_device_);

    const auto surface_format = select_swap_surface_format(swap_chain_details.formats);
    const auto present_mode = select_swap_present_mode(swap_chain_details.present_modes);
    const auto extent = select_swap_extent(swap_chain_details.capabilities);

    uint32_t image_count = swap_chain_details.capabilities.minImageCount + 1;

    if (swap_chain_details.capabilities.maxImageCount > 0 && image_count > swap_chain_details.capabilities.maxImageCount)
    {
        image_count = swap_chain_details.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    create_info.surface = surface_;
    create_info.minImageCount = image_count;
    create_info.imageFormat = surface_format.format;
    create_info.imageColorSpace = surface_format.colorSpace;
    create_info.imageExtent = extent;
    create_info.imageArrayLayers = 1;
    create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    const auto indices = find_queue_families(physical_device_);

    if (!indices.graphics_family.has_value() || !indices.present_family.has_value())
    {
        throw std::runtime_error("Error: queue family unavailable.");
    }
    
    std::vector queue_family_indices =
    {
        indices.graphics_family.value(),
        indices.present_family.value(),
    };
    
    if (indices.present_family.value() != indices.graphics_family.value())
    {
        create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        create_info.queueFamilyIndexCount = static_cast<uint32_t>(queue_family_indices.size());
        create_info.pQueueFamilyIndices = queue_family_indices.data();
    }
    else
    {
        create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    create_info.preTransform = swap_chain_details.capabilities.currentTransform;
    create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

    create_info.presentMode = present_mode;
    create_info.clipped = VK_TRUE;

    create_info.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(device_, &create_info, nullptr, &swap_chain_) != VK_SUCCESS)
    {
        throw std::runtime_error("Error: unable to create swap chain.");
    }

    vkGetSwapchainImagesKHR(device_, swap_chain_, &image_count, nullptr);
    swap_chain_images_.resize(image_count);
    vkGetSwapchainImagesKHR(device_, swap_chain_, &image_count, swap_chain_images_.data());

    swap_chain_image_format_ = surface_format.format;
    swap_chain_extent_ = extent;

    camera_->set_aspect_ratio(static_cast<float>(swap_chain_extent_.width) / static_cast<float>(swap_chain_extent_.height));
}

void GraphicsRunner::create_image_views()
{
    swap_chain_image_views_.resize(swap_chain_images_.size());

    for (size_t i = 0; i < swap_chain_images_.size(); ++i)
    {
        swap_chain_image_views_[i] = create_image_view(swap_chain_images_[i], swap_chain_image_format_, VK_IMAGE_ASPECT_COLOR_BIT, 1);
    }
}

std::vector<char> GraphicsRunner::read_file(const std::string &filename)
{
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open())
    {
        throw std::runtime_error("Error: unable to open file " + filename);
    }

    const size_t file_size = file.tellg();
    std::vector<char> buffer(file_size);
    file.seekg(0);
    file.read(buffer.data(), static_cast<long long>(file_size));
    file.close();

    return buffer;
}

VkShaderModule GraphicsRunner::create_shader_module(const std::vector<char> &code)
{
    VkShaderModuleCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    create_info.codeSize = code.size();
    create_info.pCode = reinterpret_cast<const uint32_t *>(code.data()); // TODO: learn more about why we do this

    VkShaderModule shader_module;
    if (vkCreateShaderModule(device_, &create_info, nullptr, &shader_module) != VK_SUCCESS)
    {
        throw std::runtime_error("Error: unable to create shader module.");
    }

    return shader_module;
}

void GraphicsRunner::create_render_pass()
{
    VkAttachmentDescription color_attachment{};
    color_attachment.format = swap_chain_image_format_;
    color_attachment.samples = msaa_samples_;
    // clears framebuffer to black before drawing new frame
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; 
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription depth_attachment{};
    depth_attachment.format = find_depth_format();
    depth_attachment.samples = msaa_samples_;
    depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription color_attachment_resolve{};
    color_attachment_resolve.format = swap_chain_image_format_;
    color_attachment_resolve.samples = VK_SAMPLE_COUNT_1_BIT;
    color_attachment_resolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment_resolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment_resolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment_resolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment_resolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment_resolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference color_attachment_reference{};
    color_attachment_reference.attachment = 0;
    color_attachment_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depth_attachment_reference{};
    depth_attachment_reference.attachment = 1;
    depth_attachment_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference color_attachment_resolve_reference{};
    color_attachment_resolve_reference.attachment = 2;
    color_attachment_resolve_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment_reference;
    subpass.pDepthStencilAttachment = &depth_attachment_reference;
    subpass.pResolveAttachments = &color_attachment_resolve_reference;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    std::array attachments = {color_attachment, depth_attachment, color_attachment_resolve};

    VkRenderPassCreateInfo render_pass_create_info{};
    render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_create_info.attachmentCount = static_cast<uint32_t>(attachments.size());
    render_pass_create_info.pAttachments = attachments.data();
    render_pass_create_info.subpassCount = 1;
    render_pass_create_info.pSubpasses = &subpass;
    render_pass_create_info.dependencyCount = 1;
    render_pass_create_info.pDependencies = &dependency;

    if (vkCreateRenderPass(device_, &render_pass_create_info, nullptr, &render_pass_) != VK_SUCCESS)
    {
        throw std::runtime_error("Error: unable to create render pass.");
    }
}

// if an object has multiple transformations within it, descriptorCount 
// should be more than one (i.e. a skeletal mesh could use one for each
// bone)
// the one use here means the whole object will transform
void GraphicsRunner::create_global_descriptor_set_layout()
{
    VkDescriptorSetLayoutBinding ubo_layout_binding{};
    ubo_layout_binding.binding = 0;
    ubo_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    ubo_layout_binding.descriptorCount = 1;
    ubo_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    ubo_layout_binding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo layout_create_info{};
    layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout_create_info.bindingCount = 1;
    layout_create_info.pBindings = &ubo_layout_binding;
    
    if (vkCreateDescriptorSetLayout(device_, &layout_create_info, nullptr, &global_descriptor_set_layout_) != VK_SUCCESS)
    {
        throw std::runtime_error("Error: unable to create global descriptor set layout");
    }
}

void GraphicsRunner::create_texture_descriptor_set_layout()
{
    VkDescriptorSetLayoutBinding sampler_layout_binding{};
    sampler_layout_binding.binding = 0; // One binding for the texture sampler.
    sampler_layout_binding.descriptorCount = 1;
    sampler_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    sampler_layout_binding.pImmutableSamplers = nullptr;
    sampler_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    
    VkDescriptorSetLayoutCreateInfo layout_create_info{};
    layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout_create_info.bindingCount = 1;
    layout_create_info.pBindings = &sampler_layout_binding;

    if (vkCreateDescriptorSetLayout(device_, &layout_create_info, nullptr, &texture_descriptor_set_layout_) != VK_SUCCESS)
    {
        throw std::runtime_error("Error: unable to create texture descriptor set layout");
    }
}

void GraphicsRunner::create_graphics_pipeline()
{
    auto vert_shader_code = read_file("Shaders/Vertex/vert.spv");
    auto frag_shader_code = read_file("Shaders/Fragment/frag.spv");

    auto vert_shader_module = create_shader_module(vert_shader_code);
    auto frag_shader_module = create_shader_module(frag_shader_code);

    VkPipelineShaderStageCreateInfo vert_shader_stage_create_info{};
    vert_shader_stage_create_info.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vert_shader_stage_create_info.stage  = VK_SHADER_STAGE_VERTEX_BIT;
    vert_shader_stage_create_info.module = vert_shader_module;
    vert_shader_stage_create_info.pName  = "main";
    
    VkPipelineShaderStageCreateInfo frag_shader_stage_create_info{};
    frag_shader_stage_create_info.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    frag_shader_stage_create_info.stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
    frag_shader_stage_create_info.module = frag_shader_module;
    frag_shader_stage_create_info.pName  = "main";

    VkPipelineShaderStageCreateInfo shader_stages[] = {
        vert_shader_stage_create_info,
        frag_shader_stage_create_info
    };

    VkPipelineVertexInputStateCreateInfo vertex_input_info{};
    auto binding_description = Vertex::get_binding_description();
    auto attribute_descriptions = Vertex::get_attribute_descriptions();
    vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_info.vertexBindingDescriptionCount   = 1;
    vertex_input_info.pVertexBindingDescriptions      = &binding_description;
    vertex_input_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(attribute_descriptions.size());
    vertex_input_info.pVertexAttributeDescriptions    = attribute_descriptions.data();

    VkPipelineInputAssemblyStateCreateInfo input_assembly{};
    input_assembly.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly.topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport{};
    viewport.x        = 0.0f;
    viewport.y        = 0.0f;
    viewport.width    = static_cast<float>(swap_chain_extent_.width);
    viewport.height   = static_cast<float>(swap_chain_extent_.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = swap_chain_extent_;

    std::vector<VkDynamicState> dynamic_states = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamic_state{};
    dynamic_state.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state.dynamicStateCount = static_cast<uint32_t>(dynamic_states.size());
    dynamic_state.pDynamicStates    = dynamic_states.data();

    VkPipelineViewportStateCreateInfo viewport_state{};
    viewport_state.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state.viewportCount = 1;
    viewport_state.pViewports    = &viewport;
    viewport_state.scissorCount  = 1;
    viewport_state.pScissors     = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable        = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode             = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth               = 1.0f;
    rasterizer.cullMode                = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable         = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable  = VK_TRUE;
    multisampling.rasterizationSamples = msaa_samples_;
    multisampling.minSampleShading     = 0.2f;

    VkPipelineColorBlendAttachmentState color_blend_attachment{};
    color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                              VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    color_blend_attachment.blendEnable    = VK_TRUE;
    color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    color_blend_attachment.colorBlendOp        = VK_BLEND_OP_ADD;
    color_blend_attachment.srcAlphaBlendFactor   = VK_BLEND_FACTOR_ONE;
    color_blend_attachment.dstAlphaBlendFactor   = VK_BLEND_FACTOR_ZERO;
    color_blend_attachment.alphaBlendOp          = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo color_blending{};
    color_blending.sType             = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blending.logicOpEnable     = VK_FALSE;
    color_blending.attachmentCount   = 1;
    color_blending.pAttachments      = &color_blend_attachment;
    color_blending.blendConstants[0] = 0.0f;
    color_blending.blendConstants[1] = 0.0f;
    color_blending.blendConstants[2] = 0.0f;
    color_blending.blendConstants[3] = 0.0f;

    // Add a push constant range for the per-object model matrix.
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstantRange.offset     = 0;
    pushConstantRange.size       = sizeof(glm::mat4);

    // Use two descriptor set layouts:
    // Set 0: global UBO, Set 1: texture sampler.
    std::array set_layouts = { global_descriptor_set_layout_, texture_descriptor_set_layout_ };

    VkPipelineLayoutCreateInfo pipeline_layout_create_info{};
    pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_create_info.setLayoutCount = static_cast<uint32_t>(set_layouts.size());
    pipeline_layout_create_info.pSetLayouts = set_layouts.data();
    pipeline_layout_create_info.pushConstantRangeCount = 1;
    pipeline_layout_create_info.pPushConstantRanges = &pushConstantRange;

    if (vkCreatePipelineLayout(device_, &pipeline_layout_create_info, nullptr, &pipeline_layout_) != VK_SUCCESS)
    {
        throw std::runtime_error("Error: unable to create pipeline layout.");
    }

    VkPipelineDepthStencilStateCreateInfo depth_stencil_state_create_info{};
    depth_stencil_state_create_info.sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_stencil_state_create_info.depthTestEnable       = VK_TRUE;
    depth_stencil_state_create_info.depthWriteEnable      = VK_TRUE;
    depth_stencil_state_create_info.depthCompareOp        = VK_COMPARE_OP_LESS;
    depth_stencil_state_create_info.depthBoundsTestEnable = VK_FALSE;
    depth_stencil_state_create_info.stencilTestEnable     = VK_FALSE;

    VkGraphicsPipelineCreateInfo pipeline_create_info{};
    pipeline_create_info.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_create_info.stageCount          = 2;
    pipeline_create_info.pStages             = shader_stages;
    pipeline_create_info.pVertexInputState   = &vertex_input_info;
    pipeline_create_info.pInputAssemblyState = &input_assembly;
    pipeline_create_info.pViewportState      = &viewport_state;
    pipeline_create_info.pRasterizationState = &rasterizer;
    pipeline_create_info.pMultisampleState   = &multisampling;
    pipeline_create_info.pDepthStencilState  = &depth_stencil_state_create_info;
    pipeline_create_info.pColorBlendState    = &color_blending;
    pipeline_create_info.pDynamicState       = &dynamic_state;
    pipeline_create_info.layout              = pipeline_layout_;
    pipeline_create_info.renderPass          = render_pass_;
    pipeline_create_info.subpass             = 0;
    pipeline_create_info.basePipelineHandle  = VK_NULL_HANDLE;

    if (vkCreateGraphicsPipelines(device_, VK_NULL_HANDLE, 1, &pipeline_create_info, nullptr, &graphics_pipeline_) != VK_SUCCESS)
    {
        throw std::runtime_error("Error: unable to create graphics pipeline");
    }
    
    vkDestroyShaderModule(device_, vert_shader_module, nullptr);
    vkDestroyShaderModule(device_, frag_shader_module, nullptr);
}

void GraphicsRunner::create_frame_buffers()
{
    swap_chain_framebuffers_.resize(swap_chain_image_views_.size());

    for (size_t i = 0; i < swap_chain_image_views_.size(); ++i)
    {
        std::array attachments =
        {
            color_image_view_,
            depth_image_view_,
            swap_chain_image_views_[i],
        };

        VkFramebufferCreateInfo framebuffer_create_info{};
        framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebuffer_create_info.renderPass = render_pass_;
        framebuffer_create_info.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebuffer_create_info.pAttachments = attachments.data();
        framebuffer_create_info.width = swap_chain_extent_.width;
        framebuffer_create_info.height = swap_chain_extent_.height;
        framebuffer_create_info.layers = 1;

        if (vkCreateFramebuffer(device_, &framebuffer_create_info, nullptr, &swap_chain_framebuffers_[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("Error: unable to create framebuffer.");
        }
    }
}

void GraphicsRunner::create_command_pools()
{
    auto queue_family_indices = find_queue_families(physical_device_);

    if (!queue_family_indices.graphics_family.has_value())
    {
        throw std::runtime_error("Error: graphics family not found.");
    }

    VkCommandPoolCreateInfo graphics_pool_create_info{};
    graphics_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    graphics_pool_create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    graphics_pool_create_info.queueFamilyIndex = queue_family_indices.graphics_family.value();

    if (vkCreateCommandPool(device_, &graphics_pool_create_info, nullptr, &command_pool_) != VK_SUCCESS)
    {
        throw std::runtime_error("Error: unable to create command pool."); 
    }
}

void GraphicsRunner::create_color_resources()
{
    VkFormat color_format = swap_chain_image_format_;

    create_image(swap_chain_extent_.width,
        swap_chain_extent_.height,
        1,
        msaa_samples_,
        color_format,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        color_image_,
        color_image_allocation_);

    color_image_view_ = create_image_view(color_image_, color_format, VK_IMAGE_ASPECT_COLOR_BIT, 1);
}

VkFormat GraphicsRunner::find_supported_format(const std::vector<VkFormat> &candidates,
                                               VkImageTiling tiling, VkFormatFeatureFlags features)
{
    for (auto format : candidates)
    {
        VkFormatProperties properties;
        vkGetPhysicalDeviceFormatProperties(physical_device_, format, &properties);

        if (tiling == VK_IMAGE_TILING_LINEAR && (properties.linearTilingFeatures & features) == features)
        {
            return format;
        }
        
        if (tiling == VK_IMAGE_TILING_OPTIMAL && (properties.optimalTilingFeatures & features) == features)
        {
            return format;
        }
    }

    throw std::runtime_error("Error: unable to find supported format");
}

VkFormat GraphicsRunner::find_depth_format()
{
    return find_supported_format(
        {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
}

bool GraphicsRunner::has_stencil_component(VkFormat format)
{
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

void GraphicsRunner::create_depth_resources()
{
    auto depth_format = find_depth_format();

    create_image(swap_chain_extent_.width,
        swap_chain_extent_.height,
        1,
        msaa_samples_,
        depth_format,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        depth_image_,
        depth_image_allocation_
    );

    depth_image_view_ = create_image_view(depth_image_, depth_format, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
}

void GraphicsRunner::create_image(uint32_t width, uint32_t height, uint32_t mip_levels,
    VkSampleCountFlagBits num_samples, VkFormat format, VkImageTiling tiling,
    VkImageUsageFlags usage, VkMemoryPropertyFlags properties,
    VkImage &image, VmaAllocation &image_allocation)
{
    VkImageCreateInfo image_create_info{};
    image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_create_info.imageType = VK_IMAGE_TYPE_2D;
    image_create_info.extent.width = width;
    image_create_info.extent.height = height;
    image_create_info.extent.depth = 1;
    image_create_info.mipLevels = mip_levels;
    image_create_info.arrayLayers = 1;
    image_create_info.format = format;
    image_create_info.tiling = tiling;
    image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_create_info.usage = usage;
    image_create_info.samples = num_samples;
    image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo = {};
    // Map properties to VMA usage; for images on GPU, typically GPU_ONLY.
    if (properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
    {
        allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
    }
    else
    {
        allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    }

    if (vmaCreateImage(allocator_, &image_create_info, &allocInfo, &image, &image_allocation, nullptr) != VK_SUCCESS)
    {
        throw std::runtime_error("Error: unable to create image with VMA.");
    }
}

void GraphicsRunner::generate_mip_maps(VkImage image, VkFormat image_format, int32_t texture_width, int32_t texture_height, uint32_t mip_levels)
{
    VkFormatProperties format_properties;
    vkGetPhysicalDeviceFormatProperties(physical_device_, image_format, &format_properties);

    // TODO: Add Image Resizing
    if (!(format_properties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
    {
        throw std::runtime_error("Error: texture image format does not support linear blitting.");
    }
    
    auto command_buffer = begin_single_time_commands();

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.image = image;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.levelCount = 1;

    int32_t mip_width = texture_width;
    int32_t mip_height = texture_height;

    for (uint32_t i = 1; i < mip_levels; ++i)
    {
        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        vkCmdPipelineBarrier(
            command_buffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier
        );

        VkImageBlit blit{};
        blit.srcOffsets[0] = {0, 0, 0};
        blit.srcOffsets[1] = {mip_width, mip_height, 1};
        blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.srcSubresource.mipLevel = i - 1;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = 1;
        blit.dstOffsets[0] = {0, 0, 0};
        blit.dstOffsets[1] = {mip_width > 1 ? mip_width / 2 : 1, mip_height > 1 ? mip_height / 2 : 1, 1};
        blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.dstSubresource.mipLevel = i;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = 1;

        vkCmdBlitImage(
            command_buffer,
            image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &blit,
            VK_FILTER_LINEAR
        );

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(
            command_buffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier
        );

        if (mip_width > 1)
        {
            mip_width /= 2;
        }

        if (mip_height > 1)
        {
            mip_height /= 2;
        }
    }

    barrier.subresourceRange.baseMipLevel = mip_levels - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(
        command_buffer,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );

    end_single_time_commands(command_buffer);
}

void GraphicsRunner::create_texture_image(const std::string &texture_path, RenderableResource& resource)
{
    int texture_width, texture_height, texture_channels;
    stbi_uc* pixels = stbi_load(texture_path.c_str(), &texture_width, &texture_height, &texture_channels, STBI_rgb_alpha);

    resource.mip_levels = static_cast<uint32_t>(std::floor(std::log2(std::max(texture_width, texture_height)))) + 1;

    const VkDeviceSize image_size = static_cast<uint64_t>(texture_width) * static_cast<uint64_t>(texture_height) * 4l;

    if (!pixels)
    {
        throw std::runtime_error("Error: unable to load texture image");
    }

    VkBuffer staging_buffer;
    VmaAllocation staging_buffer_allocation;
    create_buffer(image_size,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        staging_buffer,
        staging_buffer_allocation);void* data;
    
    vmaMapMemory(allocator_, staging_buffer_allocation, &data);
    memcpy(data, pixels, image_size);
    vmaUnmapMemory(allocator_, staging_buffer_allocation);

    stbi_image_free(pixels);

    create_image(texture_width, texture_height, resource.mip_levels,
        VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_SRGB,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        resource.texture_image, resource.textureImageAllocation);

    transition_image_layout(
        resource.texture_image,
        VK_FORMAT_R8G8B8A8_SRGB,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        resource.mip_levels
    );

    copy_buffer_to_image(
        staging_buffer,
        resource.texture_image,
        static_cast<uint32_t>(texture_width),
        static_cast<uint32_t>(texture_height)
    );

    vmaDestroyBuffer(allocator_, staging_buffer, staging_buffer_allocation);

    generate_mip_maps(resource.texture_image, VK_FORMAT_R8G8B8A8_SRGB, texture_width, texture_height, resource.mip_levels);
}

VkImageView GraphicsRunner::create_image_view(VkImage image, VkFormat format, VkImageAspectFlags aspect_flags, uint32_t mip_levels)
{
    VkImageViewCreateInfo view_info{};
    view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_info.image = image;
    view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view_info.format = format;
    view_info.subresourceRange.aspectMask = aspect_flags;
    view_info.subresourceRange.baseMipLevel = 0;
    view_info.subresourceRange.levelCount = mip_levels;
    view_info.subresourceRange.baseArrayLayer = 0;
    view_info.subresourceRange.layerCount = 1;

    VkImageView image_view;
    if (vkCreateImageView(device_, &view_info, nullptr, &image_view) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture image view!");
    }

    return image_view;
}

void GraphicsRunner::create_texture_image_view(RenderableResource& resource)
{
    resource.texture_image_view = create_image_view(resource.texture_image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, resource.mip_levels);
}

void GraphicsRunner::create_texture_sampler(RenderableResource& resource)
{
    VkSamplerCreateInfo sampler_create_info{};
    sampler_create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_create_info.magFilter = VK_FILTER_LINEAR;
    sampler_create_info.minFilter = VK_FILTER_LINEAR;
    // what to do when we get to the edge of the image:
    // - repeat
    // - mirrored repeat
    // - clamp to edge
    // - mirror clamp to edge
    // - clamp to border
    sampler_create_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_create_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_create_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_create_info.anisotropyEnable = VK_TRUE;

    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(physical_device_, &properties);
    
    sampler_create_info.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
    sampler_create_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    sampler_create_info.unnormalizedCoordinates = VK_FALSE;
    sampler_create_info.compareEnable = VK_FALSE;
    sampler_create_info.compareOp = VK_COMPARE_OP_ALWAYS;
    sampler_create_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler_create_info.mipLodBias = 0.0f;
    sampler_create_info.minLod = 0.0f;
    sampler_create_info.maxLod = static_cast<float>(resource.mip_levels);

    if (vkCreateSampler(device_, &sampler_create_info, nullptr, &resource.texture_sampler) != VK_SUCCESS)
    {
        throw std::runtime_error("Error: unable to create texture sampler.");
    }
}

uint32_t GraphicsRunner::find_memory_type(uint32_t type_filter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memory_properties;
    vkGetPhysicalDeviceMemoryProperties(physical_device_, &memory_properties);

    for (uint32_t i = 0; i < memory_properties.memoryTypeCount; ++i)
    {
        if ((type_filter & (1 << i)) && (memory_properties.memoryTypes[i].propertyFlags & properties) == properties)
        {
            return i;
        }
    }

    throw std::runtime_error("Error: unable to find suitable memory type.");
}

void GraphicsRunner::create_buffer(VkDeviceSize size, VkBufferUsageFlags usage,
    VkMemoryPropertyFlags properties, VkBuffer& buffer, VmaAllocation &buffer_allocation)
{
    VkBufferCreateInfo buffer_create_info{};
    buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_create_info.size = size;
    buffer_create_info.usage = usage;
    buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo = {};
    // Simple mapping: if host-visible then use CPU_TO_GPU; otherwise, GPU_ONLY.
    if (properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
    {
        allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
    }
    else
    {
        allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    }

    if (vmaCreateBuffer(allocator_, &buffer_create_info, &allocInfo, &buffer, &buffer_allocation, nullptr) != VK_SUCCESS)
    {
        throw std::runtime_error("Error: unable to create buffer with VMA.");
    }
}

void GraphicsRunner::transition_image_layout(VkImage image, VkFormat format, VkImageLayout old_layout, VkImageLayout new_layout, uint32_t mip_levels)
{
    VkCommandBuffer command_buffer = begin_single_time_commands();

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = old_layout;
    barrier.newLayout = new_layout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;

    if (new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
    {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

        if (has_stencil_component(format))
        {
            barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
    }
    else
    {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }
    
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = mip_levels;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags source_stage;
    VkPipelineStageFlags destination_stage;

    if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destination_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destination_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destination_stage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    }
    else
    {
        throw std::invalid_argument("Error: layout transition not supported");
    }

    vkCmdPipelineBarrier(
        command_buffer,
        source_stage, destination_stage,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );

    end_single_time_commands(command_buffer);
}

void GraphicsRunner::copy_buffer_to_image(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
{
    auto command_buffer = begin_single_time_commands();

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;

    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;

    region.imageOffset = {0, 0, 0};
    region.imageExtent = {width, height, 1};

    vkCmdCopyBufferToImage(
        command_buffer,
        buffer,
        image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &region
    );

    end_single_time_commands(command_buffer);
}

VkCommandBuffer GraphicsRunner::begin_single_time_commands()
{
    VkCommandBufferAllocateInfo allocate_info{};
    allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocate_info.commandPool = command_pool_;
    allocate_info.commandBufferCount = 1;

    VkCommandBuffer command_buffer;
    vkAllocateCommandBuffers(device_, &allocate_info, &command_buffer);

    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(command_buffer, &begin_info);

    return command_buffer;
}

void GraphicsRunner::end_single_time_commands(VkCommandBuffer command_buffer)
{
    vkEndCommandBuffer(command_buffer);

    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer;

    vkQueueSubmit(graphics_queue_, 1, &submit_info, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphics_queue_);

    vkFreeCommandBuffers(device_, command_pool_, 1, &command_buffer);
}

void GraphicsRunner::copy_buffer(VkBuffer src_buffer, VkBuffer dst_buffer, VkDeviceSize size)
{
    auto command_buffer = begin_single_time_commands();

    VkBufferCopy copy_region{};
    copy_region.srcOffset = 0;
    copy_region.dstOffset = 0;
    copy_region.size = size;
    vkCmdCopyBuffer(command_buffer, src_buffer, dst_buffer, 1, &copy_region);

    end_single_time_commands(command_buffer);
}

void GraphicsRunner::create_uniform_buffers()
{
    VkDeviceSize buffer_size = sizeof(UniformBufferObject);

    uniform_buffers_.resize(max_frames_in_flight_);
    uniform_buffers_allocations_.resize(max_frames_in_flight_);
    uniform_buffers_mapped_.resize(max_frames_in_flight_);

    for (size_t i = 0; i < max_frames_in_flight_; ++i)
    {
        create_buffer(buffer_size,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            uniform_buffers_[i],
            uniform_buffers_allocations_[i]);

        vmaMapMemory(allocator_, uniform_buffers_allocations_[i], &uniform_buffers_mapped_[i]);
    }
}

void GraphicsRunner::create_descriptor_pool()
{
    std::array<VkDescriptorPoolSize, 2> pool_sizes{};
    pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    pool_sizes[0].descriptorCount = static_cast<uint32_t>(max_frames_in_flight_);
    pool_sizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    // Global sets + support for up to 100 texture sets.
    pool_sizes[1].descriptorCount = static_cast<uint32_t>(max_frames_in_flight_ + 100);

    VkDescriptorPoolCreateInfo pool_create_info{};
    pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_create_info.poolSizeCount = static_cast<uint32_t>(pool_sizes.size());
    pool_create_info.pPoolSizes = pool_sizes.data();
    pool_create_info.maxSets = static_cast<uint32_t>(max_frames_in_flight_ + 100);

    if (vkCreateDescriptorPool(device_, &pool_create_info, nullptr, &descriptor_pool_) != VK_SUCCESS)
    {
        throw std::runtime_error("Error: unable to create descriptor pool.");
    }
}

void GraphicsRunner::create_descriptor_sets()
{
    std::vector layouts(max_frames_in_flight_, global_descriptor_set_layout_);

    VkDescriptorSetAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorPool = descriptor_pool_;
    alloc_info.descriptorSetCount = static_cast<uint32_t>(max_frames_in_flight_);
    alloc_info.pSetLayouts = layouts.data();

    descriptor_sets_.resize(max_frames_in_flight_);
    if (vkAllocateDescriptorSets(device_, &alloc_info, descriptor_sets_.data()) != VK_SUCCESS) {
        throw std::runtime_error("Error: unable to allocate global descriptor sets.");
    }

    for (size_t i = 0; i < max_frames_in_flight_; ++i)
    {
        VkDescriptorBufferInfo buffer_info{};
        buffer_info.buffer = uniform_buffers_[i];
        buffer_info.offset = 0;
        buffer_info.range = sizeof(UniformBufferObject);
        
        VkWriteDescriptorSet descriptor_write{};
        descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptor_write.dstSet = descriptor_sets_[i];
        descriptor_write.dstBinding = 0;
        descriptor_write.dstArrayElement = 0;
        descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptor_write.descriptorCount = 1;
        descriptor_write.pBufferInfo = &buffer_info;
        
        vkUpdateDescriptorSets(device_, 1, &descriptor_write, 0, nullptr);
    }
}

void GraphicsRunner::create_command_buffers()
{
    command_buffers_.resize(max_frames_in_flight_);
    
    VkCommandBufferAllocateInfo allocate_info{};
    allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocate_info.commandPool = command_pool_;
    allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocate_info.commandBufferCount = static_cast<uint32_t>(command_buffers_.size());

    if (vkAllocateCommandBuffers(device_, &allocate_info, command_buffers_.data()) != VK_SUCCESS)
    {
        throw std::runtime_error("Error: unable to create command buffer.");
    }
}

void GraphicsRunner::create_sync_objects()
{
    image_available_semaphores_.resize(max_frames_in_flight_);
    render_finished_semaphores_.resize(max_frames_in_flight_);
    in_flight_fences_.resize(max_frames_in_flight_);

    
    VkSemaphoreCreateInfo semaphore_create_info{};
    semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fence_create_info{};
    fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < max_frames_in_flight_; ++i)
    {
        if (vkCreateSemaphore(device_, &semaphore_create_info, nullptr, &image_available_semaphores_[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("Error: unable to create image available semaphore.");
        }

        if (vkCreateSemaphore(device_, &semaphore_create_info, nullptr, &render_finished_semaphores_[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("Error: unable to create render finished semaphore.");
        }

        if (vkCreateFence(device_, &fence_create_info, nullptr, &in_flight_fences_[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("Error: unable to create in flight fence.");
        }
    }
}

void GraphicsRunner::record_command_buffer(VkCommandBuffer command_buffer, uint32_t image_index)
{
    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = 0;
    begin_info.pInheritanceInfo = nullptr;

    if (vkBeginCommandBuffer(command_buffer, &begin_info) != VK_SUCCESS)
    {
        throw std::runtime_error("Error: unable to begin command buffer.");
    }

    std::array<VkClearValue, 2> clear_values{};
    clear_values[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
    clear_values[1].depthStencil = {1.0f, 0};

    VkRenderPassBeginInfo render_pass_begin_info{};
    render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_begin_info.renderPass = render_pass_;
    render_pass_begin_info.framebuffer = swap_chain_framebuffers_[image_index];
    render_pass_begin_info.renderArea.offset = {0, 0};
    render_pass_begin_info.renderArea.extent = swap_chain_extent_;
    render_pass_begin_info.clearValueCount = static_cast<uint32_t>(clear_values.size());
    render_pass_begin_info.pClearValues = clear_values.data();
    
    vkCmdBeginRenderPass(command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline_);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(swap_chain_extent_.width);
    viewport.height = static_cast<float>(swap_chain_extent_.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(command_buffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = swap_chain_extent_;
    vkCmdSetScissor(command_buffer, 0, 1, &scissor);

    // Bind global descriptor set (set 0: camera UBO)
    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                              pipeline_layout_, 0, 1, &descriptor_sets_[current_frame_], 0, nullptr);

    // For each resource, bind vertex/index buffers, bind its texture descriptor set (set 1),
    // push its model matrix, and draw.
    for (const auto &resource : resources_ | std::views::values) {
        const VkBuffer vertex_buffers[] = { resource.vertexBuffer };
        constexpr VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(command_buffer, 0, 1, vertex_buffers, offsets);
        vkCmdBindIndexBuffer(command_buffer, resource.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
        
        // Bind resource’s texture descriptor set at set index 1.
        vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout_,
                                  1, 1, &resource.texture_descriptor_set, 0, nullptr);
        
        // Push the per-resource model matrix via push constants.
        vkCmdPushConstants(command_buffer, pipeline_layout_,
                           VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &resource.model);
        
        vkCmdDrawIndexed(command_buffer, static_cast<uint32_t>(resource.indices.size()), 1, 0, 0, 0);
    }

    vkCmdEndRenderPass(command_buffer);

    if (vkEndCommandBuffer(command_buffer) != VK_SUCCESS)
    {
        throw std::runtime_error("Error: unable to record command buffer.");
    }
}

void GraphicsRunner::draw_frame()
{
    vkWaitForFences(device_, 1, &in_flight_fences_[current_frame_], VK_TRUE, UINT64_MAX);

    uint32_t image_index;
    auto result = vkAcquireNextImageKHR(device_, swap_chain_, UINT64_MAX, image_available_semaphores_[current_frame_], VK_NULL_HANDLE, &image_index);

    if (result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        recreate_swap_chain();
        return;
    }
    
    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
    {
        throw std::runtime_error("Error: failed to acquire swap chain image.");
    }

    vkResetFences(device_, 1, &in_flight_fences_[current_frame_]);
    
    vkResetCommandBuffer(command_buffers_[current_frame_], 0);

    record_command_buffer(command_buffers_[current_frame_], image_index);

    update_uniform_buffer();

    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore wait_semaphores[] = {image_available_semaphores_[current_frame_]};
    VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = wait_semaphores;
    submit_info.pWaitDstStageMask = wait_stages;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffers_[current_frame_];

    VkSemaphore signal_semaphores[] = {render_finished_semaphores_[current_frame_]};
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = signal_semaphores;

    if (vkQueueSubmit(graphics_queue_, 1, &submit_info, in_flight_fences_[current_frame_]) != VK_SUCCESS)
    {
        throw std::runtime_error("Error: unable to submit draw command buffer.");
    }

    VkPresentInfoKHR present_info{};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = signal_semaphores;

    VkSwapchainKHR swap_chains[] = {swap_chain_};
    present_info.swapchainCount = 1;
    present_info.pSwapchains = swap_chains;
    present_info.pImageIndices = &image_index;
    // only needed for multiple swap chains
    present_info.pResults = nullptr;

    result = vkQueuePresentKHR(present_queue_, &present_info);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || frame_buffer_resized)
    {
        frame_buffer_resized = false;
        recreate_swap_chain();
    }
    else if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Error: unable to present swap chain image.");
    }

    current_frame_ = (current_frame_ + 1) % max_frames_in_flight_;
}

void GraphicsRunner::update_uniform_buffer()
{
    const UniformBufferObject ubo = camera_->get_ubo();

    memcpy(uniform_buffers_mapped_[current_frame_], &ubo, sizeof(ubo));
}

void GraphicsRunner::clean_up()
{
    // wait for last frame & stuff to process
    vkDeviceWaitIdle(device_);
    
    clean_up_swap_chain();

    for (size_t i = 0; i < max_frames_in_flight_; ++i)
    {
        vmaUnmapMemory(allocator_, uniform_buffers_allocations_[i]);
        vmaDestroyBuffer(allocator_, uniform_buffers_[i], uniform_buffers_allocations_[i]);
    }

    vkDestroyDescriptorPool(device_, descriptor_pool_, nullptr);

    vkDestroyDescriptorSetLayout(device_, global_descriptor_set_layout_, nullptr);

    vkDestroyDescriptorSetLayout(device_, texture_descriptor_set_layout_, nullptr);

    for (const auto &resource : resources_ | std::views::values)
    {
        vkDestroySampler(device_, resource.texture_sampler, nullptr);
        vkDestroyImageView(device_, resource.texture_image_view, nullptr);
        vmaDestroyImage(allocator_, resource.texture_image, resource.textureImageAllocation);

        vmaDestroyBuffer(allocator_, resource.vertexBuffer, resource.vertexBufferAllocation);
        vmaDestroyBuffer(allocator_, resource.indexBuffer, resource.indexBufferAllocation);
    }

    resources_.clear();
    
    for (size_t i = 0; i < max_frames_in_flight_; ++i)
    {
        vkDestroySemaphore(device_, image_available_semaphores_[i], nullptr);
        vkDestroySemaphore(device_, render_finished_semaphores_[i], nullptr);
        vkDestroyFence(device_, in_flight_fences_[i], nullptr);
    }
    
    vkDestroyCommandPool(device_, command_pool_, nullptr);
    
    vkDestroyPipeline(device_, graphics_pipeline_, nullptr);
    
    vkDestroyPipelineLayout(device_, pipeline_layout_, nullptr);

    vkDestroyRenderPass(device_, render_pass_, nullptr);

    vmaDestroyAllocator(allocator_);
    
    vkDestroyDevice(device_, nullptr);
    
    if (enable_validation_layers_)
    {
        destroy_debug_utils_messenger(instance_, debug_messenger_, nullptr);
    }
    
    vkDestroySurfaceKHR(instance_, surface_, nullptr);
    
    vkDestroyInstance(instance_, nullptr);
    
    glfwDestroyWindow(window_);
    
    glfwTerminate();
}

bool GraphicsRunner::done()
{
    glfwPollEvents();

    return glfwWindowShouldClose(window_);
}

void GraphicsRunner::clean_up_swap_chain()
{
    vkDestroyImageView(device_, color_image_view_, nullptr);
    vmaDestroyImage(allocator_, color_image_, color_image_allocation_);

    vkDestroyImageView(device_, depth_image_view_, nullptr);
    vmaDestroyImage(allocator_, depth_image_, depth_image_allocation_);
    
    for (const auto framebuffer : swap_chain_framebuffers_)
    {
        vkDestroyFramebuffer(device_, framebuffer, nullptr);
    }
    
    for (const auto image_view : swap_chain_image_views_)
    {
        vkDestroyImageView(device_, image_view, nullptr);
    }
    
    vkDestroySwapchainKHR(device_, swap_chain_, nullptr);
}
