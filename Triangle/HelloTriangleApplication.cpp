// ReSharper disable CppMemberFunctionMayBeConst
// ReSharper disable CppUseStructuredBinding

// ReSharper disable CppTooWideScopeInitStatement
#include "HelloTriangleApplication.h"

#include <algorithm>
#include <stdexcept>
#include <vector>
#include <format>
#include <fstream>
#include <set>
#include <chrono>

#include "../Logging/Logging.h"
#include "../Rendering/UniformBufferObject.h"
#include "../Rendering/Vertex.h"

HelloTriangleApplication::HelloTriangleApplication() :
    window_(nullptr), instance_(), debug_messenger_(), device_(),
    graphics_queue_(), present_queue_(), surface_(), swap_chain_(),
    swap_chain_image_format_(), swap_chain_extent_(), render_pass_(),
    pipeline_layout_(), graphics_pipeline_(), command_pool_() {}

HelloTriangleApplication::~HelloTriangleApplication() = default;

void HelloTriangleApplication::run()
{
    init_window();
    init_vulkan();
    main_loop();
    clean_up();
}

void HelloTriangleApplication::frame_buffer_resize_callback(GLFWwindow *window, int width, int height)
{
    const auto app = static_cast<HelloTriangleApplication*>(glfwGetWindowUserPointer(window));
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

void HelloTriangleApplication::init_window()
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

void HelloTriangleApplication::init_vulkan()
{
    create_instance();
    set_up_debug_messenger();
    create_surface();
    select_physical_device();
    create_logical_device();
    create_swap_chain();
    create_image_views();
    create_render_pass();
    create_descriptor_set_layout();
    create_graphics_pipeline();
    create_frame_buffers();
    create_command_pools();
    create_vertex_buffer();
    create_index_buffer();
    create_uniform_buffers();
    create_descriptor_pool();
    create_descriptor_sets();
    create_command_buffers();
    create_sync_objects();
}

void HelloTriangleApplication::create_instance()
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
        create_info.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debug_create_info;
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

void HelloTriangleApplication::populate_debug_messenger_create_info(VkDebugUtilsMessengerCreateInfoEXT& create_info)
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

void HelloTriangleApplication::set_up_debug_messenger()
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

std::vector<const char *> HelloTriangleApplication::get_required_extensions()
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

bool HelloTriangleApplication::are_extensions_supported(const std::vector<const char*> &glfw_extensions)
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

bool HelloTriangleApplication::are_validation_layers_supported()
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

bool HelloTriangleApplication::are_device_extensions_supported(const VkPhysicalDevice device)
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

SwapChainSupportDetails HelloTriangleApplication::get_swap_chain_support_details(VkPhysicalDevice device)
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

int HelloTriangleApplication::rate_device(const VkPhysicalDevice device)
{
    VkPhysicalDeviceProperties device_properties;
    vkGetPhysicalDeviceProperties(device, &device_properties);

    VkPhysicalDeviceFeatures device_features;
    vkGetPhysicalDeviceFeatures(device, &device_features);

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

void HelloTriangleApplication::select_physical_device()
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

    if (highest_rating == 0 || physical_device_ == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Error: no suitable GPUs found.");
    }
}

QueueFamilyIndices HelloTriangleApplication::find_queue_families(VkPhysicalDevice device)
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

        bool transfer_support = false;
        if (queue_family.queueFlags & VK_QUEUE_TRANSFER_BIT)
        {
            transfer_support = true;
        }
        
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

        // set transfer family (as long as it hasn't already been set)
        if (transfer_support && !graphics_support && !indices.transfer_family.has_value())
        {
            indices.transfer_family = i;
        }
        
        i++;
    }

    return indices;
}

void HelloTriangleApplication::create_logical_device()
{
    const auto indices = find_queue_families(physical_device_);
    
    if (!indices.graphics_family.has_value() || !indices.present_family.has_value() || !indices.transfer_family.has_value())
    {
        throw std::runtime_error("Error: queue family not found");
    }

    std::set queue_families =
    {
        indices.graphics_family.value(),
        indices.present_family.value(),
        indices.transfer_family.value(),
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
    vkGetDeviceQueue(device_, indices.transfer_family.value(), 0, &transfer_queue_);
}

void HelloTriangleApplication::create_surface()
{
    if (glfwCreateWindowSurface(instance_, window_, nullptr, &surface_) != VK_SUCCESS)
    {
        throw std::runtime_error("Error: unable to create surface.");
    }
}

VkSurfaceFormatKHR HelloTriangleApplication::select_swap_surface_format(const std::vector<VkSurfaceFormatKHR> &available_formats)
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

VkPresentModeKHR HelloTriangleApplication::select_swap_present_mode(const std::vector<VkPresentModeKHR>& available_present_modes)
{
    for (const auto& available_present_mode : available_present_modes)
    {
        if (available_present_mode == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            return available_present_mode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D HelloTriangleApplication::select_swap_extent(const VkSurfaceCapabilitiesKHR &capabilities)
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

void HelloTriangleApplication::recreate_swap_chain()
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
    create_frame_buffers();
}

void HelloTriangleApplication::create_swap_chain()
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

    if (!indices.graphics_family.has_value() || !indices.present_family.has_value() || !indices.transfer_family.has_value())
    {
        throw std::runtime_error("Error: queue family unavailable.");
    }
    
    std::vector queue_family_indices =
    {
        indices.graphics_family.value(),
        indices.transfer_family.value(),
    };

    if (indices.present_family.value() != indices.graphics_family.value())
    {
        queue_family_indices.push_back(indices.present_family.value());
    }
    
    create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    create_info.queueFamilyIndexCount = static_cast<uint32_t>(queue_family_indices.size());
    create_info.pQueueFamilyIndices = queue_family_indices.data();

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
}

void HelloTriangleApplication::create_image_views()
{
    swap_chain_image_views_.resize(swap_chain_images_.size());

    for (size_t i = 0; i < swap_chain_images_.size(); ++i)
    {
        VkImageViewCreateInfo create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        create_info.image = swap_chain_images_[i];
        create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        create_info.format = swap_chain_image_format_;
        create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        create_info.subresourceRange.baseMipLevel = 0;
        create_info.subresourceRange.levelCount = 1;
        create_info.subresourceRange.baseArrayLayer = 0;
        create_info.subresourceRange.layerCount = 1;

        if (vkCreateImageView(device_, &create_info, nullptr, &swap_chain_image_views_[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("Error: unable to create image view.");
        }
    }
}

std::vector<char> HelloTriangleApplication::read_file(const std::string &filename)
{
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open())
    {
        throw std::runtime_error("Error: unable to open file " + filename);
    }

    const size_t file_size = file.tellg();
    std::vector<char> buffer(file_size);
    file.seekg(0);
    file.read(buffer.data(), file_size);
    file.close();

    return buffer;
}

VkShaderModule HelloTriangleApplication::create_shader_module(const std::vector<char> &code)
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

void HelloTriangleApplication::create_render_pass()
{
    VkAttachmentDescription color_attachment{};
    color_attachment.format = swap_chain_image_format_;
    // not doing multisampling yet so 1 is fine
    color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    // clears framebuffer to black before drawing new frame
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; 
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference color_attachment_reference{};
    color_attachment_reference.attachment = 0;
    color_attachment_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment_reference;

    VkRenderPassCreateInfo render_pass_create_info{};
    render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_create_info.attachmentCount = 1;
    render_pass_create_info.pAttachments = &color_attachment;
    render_pass_create_info.subpassCount = 1;
    render_pass_create_info.pSubpasses = &subpass;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    render_pass_create_info.dependencyCount = 1;
    render_pass_create_info.pDependencies = &dependency;

    if (vkCreateRenderPass(device_, &render_pass_create_info, nullptr, &render_pass_) != VK_SUCCESS)
    {
        throw std::runtime_error("Error: unable to create render pass.");
    }
}

void HelloTriangleApplication::create_descriptor_set_layout()
{
    VkDescriptorSetLayoutBinding ubo_layout_binding{};
    ubo_layout_binding.binding = 0;
    ubo_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    // if an object has multiple transformations within it, this should
    // be more than one (i.e. a skeletal mesh could use one for each
    // bone)
    // the one use here means the whole object will transform
    ubo_layout_binding.descriptorCount = 1;
    ubo_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    ubo_layout_binding.pImmutableSamplers = nullptr;
    
    VkDescriptorSetLayoutCreateInfo layout_create_info{};
    layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout_create_info.bindingCount = 1;
    layout_create_info.pBindings = &ubo_layout_binding;

    if (vkCreateDescriptorSetLayout(device_, &layout_create_info, nullptr, &descriptor_set_layout_) != VK_SUCCESS)
    {
        throw std::runtime_error("Error: unable to create descriptor set layout");
    }
}

void HelloTriangleApplication::create_graphics_pipeline()
{
    auto vert_shader_code = read_file("Shaders/Vertex/vert.spv");
    auto frag_shader_code = read_file("Shaders/Fragment/frag.spv");

    auto vert_shader_module = create_shader_module(vert_shader_code);
    auto frag_shader_module = create_shader_module(frag_shader_code);

    VkPipelineShaderStageCreateInfo vert_shader_stage_create_info{};
    vert_shader_stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vert_shader_stage_create_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vert_shader_stage_create_info.module = vert_shader_module;
    vert_shader_stage_create_info.pName = "main";
    
    VkPipelineShaderStageCreateInfo frag_shader_stage_create_info{};
    frag_shader_stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    frag_shader_stage_create_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    frag_shader_stage_create_info.module = frag_shader_module;
    frag_shader_stage_create_info.pName = "main";

    VkPipelineShaderStageCreateInfo shader_stages[] =
    {
        vert_shader_stage_create_info,
        frag_shader_stage_create_info
    };

    VkPipelineVertexInputStateCreateInfo vertex_input_info{};

    auto binding_description = Vertex::get_binding_description();
    auto attribute_descriptions = Vertex::get_attribute_descriptions();
    
    vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_info.vertexBindingDescriptionCount = 1;
    vertex_input_info.pVertexBindingDescriptions = &binding_description;
    vertex_input_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(attribute_descriptions.size());
    vertex_input_info.pVertexAttributeDescriptions = attribute_descriptions.data();

    VkPipelineInputAssemblyStateCreateInfo input_assembly{};
    input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(swap_chain_extent_.width);
    viewport.height = static_cast<float>(swap_chain_extent_.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = swap_chain_extent_;

    std::vector dynamic_states =
    {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamic_state{};
    dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state.dynamicStateCount = static_cast<uint32_t>(dynamic_states.size());
    dynamic_state.pDynamicStates = dynamic_states.data();

    VkPipelineViewportStateCreateInfo viewport_state{};
    viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state.viewportCount = 1;
    viewport_state.pViewports = &viewport;
    viewport_state.scissorCount = 1;
    viewport_state.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.0f;
    rasterizer.depthBiasClamp = 0.0f;
    rasterizer.depthBiasSlopeFactor = 0.0f;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.minSampleShading = 1.0f;
    multisampling.pSampleMask = nullptr;
    multisampling.alphaToCoverageEnable = VK_FALSE;
    multisampling.alphaToOneEnable = VK_FALSE;

    // currently blends according to alpha value
    VkPipelineColorBlendAttachmentState color_blend_attachment{};
    color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    color_blend_attachment.blendEnable = VK_TRUE;
    color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
    color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo color_blending{};
    color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blending.logicOpEnable = VK_FALSE;
    color_blending.logicOp = VK_LOGIC_OP_COPY;
    color_blending.attachmentCount = 1;
    color_blending.pAttachments = &color_blend_attachment;
    color_blending.blendConstants[0] = 0.0f;
    color_blending.blendConstants[1] = 0.0f;
    color_blending.blendConstants[2] = 0.0f;
    color_blending.blendConstants[3] = 0.0f;

    VkPipelineLayoutCreateInfo pipeline_layout_create_info{};
    pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_create_info.setLayoutCount = 1;
    pipeline_layout_create_info.pSetLayouts = &descriptor_set_layout_;
    pipeline_layout_create_info.pushConstantRangeCount = 0;
    pipeline_layout_create_info.pPushConstantRanges = nullptr;

    if (vkCreatePipelineLayout(device_, &pipeline_layout_create_info, nullptr, &pipeline_layout_) != VK_SUCCESS)
    {
        throw std::runtime_error("Error: unable to create pipeline layout.");
    }

    VkGraphicsPipelineCreateInfo pipeline_create_info{};
    pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_create_info.stageCount = 2;
    pipeline_create_info.pStages = shader_stages;
    pipeline_create_info.pVertexInputState = &vertex_input_info;
    pipeline_create_info.pInputAssemblyState = &input_assembly;
    pipeline_create_info.pViewportState = &viewport_state;
    pipeline_create_info.pRasterizationState = &rasterizer;
    pipeline_create_info.pMultisampleState = &multisampling;
    pipeline_create_info.pDepthStencilState = nullptr;
    pipeline_create_info.pColorBlendState = &color_blending;
    pipeline_create_info.pDynamicState = &dynamic_state;
    pipeline_create_info.layout = pipeline_layout_;
    pipeline_create_info.renderPass = render_pass_;
    pipeline_create_info.subpass = 0;
    pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
    pipeline_create_info.basePipelineIndex = -1;

    if (vkCreateGraphicsPipelines(device_, VK_NULL_HANDLE, 1, &pipeline_create_info, nullptr, &graphics_pipeline_) != VK_SUCCESS)
    {
        throw std::runtime_error("Error: unable to create graphics pipeline");
    }
    
    vkDestroyShaderModule(device_, vert_shader_module, nullptr);
    vkDestroyShaderModule(device_, frag_shader_module, nullptr);
}

void HelloTriangleApplication::create_frame_buffers()
{
    swap_chain_framebuffers_.resize(swap_chain_image_views_.size());

    for (size_t i = 0; i < swap_chain_image_views_.size(); ++i)
    {
        VkImageView attachments[] =
        {
            swap_chain_image_views_[i],
        };

        VkFramebufferCreateInfo framebuffer_create_info{};
        framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebuffer_create_info.renderPass = render_pass_;
        framebuffer_create_info.attachmentCount = 1;
        framebuffer_create_info.pAttachments = attachments;
        framebuffer_create_info.width = swap_chain_extent_.width;
        framebuffer_create_info.height = swap_chain_extent_.height;
        framebuffer_create_info.layers = 1;

        if (vkCreateFramebuffer(device_, &framebuffer_create_info, nullptr, &swap_chain_framebuffers_[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("Error: unable to create framebuffer.");
        }
    }
}

void HelloTriangleApplication::create_command_pools()
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

    if (!queue_family_indices.transfer_family.has_value())
    {
        throw std::runtime_error("Error: transfer family not found.");
    }

    VkCommandPoolCreateInfo transfer_pool_create_info{};
    transfer_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    transfer_pool_create_info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    transfer_pool_create_info.queueFamilyIndex = queue_family_indices.transfer_family.value();

    if (vkCreateCommandPool(device_, &transfer_pool_create_info, nullptr, &transfer_command_pool_) != VK_SUCCESS)
    {
        throw std::runtime_error("Error: unable to create command pool."); 
    }
}

uint32_t HelloTriangleApplication::find_memory_type(uint32_t type_filter, VkMemoryPropertyFlags properties)
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

void HelloTriangleApplication::create_buffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& buffer_memory)
{
    VkBufferCreateInfo buffer_create_info{};
    buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_create_info.size = size;
    buffer_create_info.usage = usage;
    buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device_, &buffer_create_info, nullptr, &buffer) != VK_SUCCESS)
    {
        throw std::runtime_error("Error: unable to create vertex buffer.");
    }

    VkMemoryRequirements memory_requirements;
    vkGetBufferMemoryRequirements(device_, buffer, &memory_requirements);

    VkMemoryAllocateInfo allocate_info{};
    allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocate_info.allocationSize = memory_requirements.size;
    allocate_info.memoryTypeIndex = find_memory_type(memory_requirements.memoryTypeBits, properties);
    
    // TODO: Custom Allocator
    if (vkAllocateMemory(device_, &allocate_info, nullptr, &buffer_memory) != VK_SUCCESS)
    {
        throw std::runtime_error("Error: unable to allocate vertex buffer memory");
    }

    vkBindBufferMemory(device_, buffer, buffer_memory, 0);
}

void HelloTriangleApplication::copy_buffer(VkBuffer src_buffer, VkBuffer dst_buffer, VkDeviceSize size)
{
    VkCommandBufferAllocateInfo allocate_info{};
    allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocate_info.commandPool = transfer_command_pool_;
    allocate_info.commandBufferCount = 1;

    VkCommandBuffer command_buffer;
    vkAllocateCommandBuffers(device_, &allocate_info, &command_buffer);

    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(command_buffer, &begin_info);

    VkBufferCopy copy_region{};
    copy_region.srcOffset = 0;
    copy_region.dstOffset = 0;
    copy_region.size = size;
    vkCmdCopyBuffer(command_buffer, src_buffer, dst_buffer, 1, &copy_region);

    vkEndCommandBuffer(command_buffer);

    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer;

    vkQueueSubmit(transfer_queue_, 1, &submit_info, VK_NULL_HANDLE);
    vkQueueWaitIdle(transfer_queue_);

    vkFreeCommandBuffers(device_, transfer_command_pool_, 1, &command_buffer);
}

void HelloTriangleApplication::create_vertex_buffer()
{
    VkDeviceSize buffer_size = sizeof(test_shape[0]) * test_shape.size();

    VkBuffer staging_buffer;
    VkDeviceMemory staging_buffer_memory;
    
    create_buffer(
        buffer_size,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        staging_buffer,
        staging_buffer_memory
    );

    void* data;
    vkMapMemory(device_, staging_buffer_memory, 0, buffer_size, 0, &data);
        memcpy(data, test_shape.data(), buffer_size);
    vkUnmapMemory(device_, staging_buffer_memory);

    create_buffer(
        buffer_size,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        vertex_buffer_,
        vertex_buffer_memory_
    );

    copy_buffer(staging_buffer, vertex_buffer_, buffer_size);

    vkDestroyBuffer(device_, staging_buffer, nullptr);
    vkFreeMemory(device_, staging_buffer_memory, nullptr);
}

void HelloTriangleApplication::create_index_buffer()
{
    VkDeviceSize buffer_size = sizeof(test_shape_indices[0]) * test_shape_indices.size();

    VkBuffer staging_buffer;
    VkDeviceMemory staging_buffer_memory;

    create_buffer(
        buffer_size,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        staging_buffer,
        staging_buffer_memory
    );

    void* data;
    vkMapMemory(device_, staging_buffer_memory, 0, buffer_size, 0, &data);
    memcpy(data, test_shape_indices.data(), buffer_size);
    vkUnmapMemory(device_, staging_buffer_memory);

    create_buffer(
        buffer_size,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        index_buffer_,
        index_buffer_memory_
    );

    copy_buffer(staging_buffer, index_buffer_, buffer_size);

    vkDestroyBuffer(device_, staging_buffer, nullptr);
    vkFreeMemory(device_, staging_buffer_memory, nullptr);
}

void HelloTriangleApplication::create_uniform_buffers()
{
    VkDeviceSize buffer_size = sizeof(UniformBufferObject);

    uniform_buffers_.resize(max_frames_in_flight_);
    uniform_buffers_memories_.resize(max_frames_in_flight_);
    uniform_buffers_mapped_.resize(max_frames_in_flight_);

    for (size_t i = 0; i < max_frames_in_flight_; ++i)
    {
        create_buffer(
            buffer_size,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            uniform_buffers_[i],
            uniform_buffers_memories_[i]
        );

        vkMapMemory(device_, uniform_buffers_memories_[i], 0, buffer_size, 0, &uniform_buffers_mapped_[i]);
    }
}

void HelloTriangleApplication::create_descriptor_pool()
{
    VkDescriptorPoolSize pool_size{};
    pool_size.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    pool_size.descriptorCount = static_cast<uint32_t>(max_frames_in_flight_);

    VkDescriptorPoolCreateInfo pool_create_info{};
    pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_create_info.poolSizeCount = 1;
    pool_create_info.pPoolSizes = &pool_size;
    pool_create_info.maxSets = static_cast<uint32_t>(max_frames_in_flight_);

    if (vkCreateDescriptorPool(device_, &pool_create_info, nullptr, &descriptor_pool_) != VK_SUCCESS)
    {
        throw std::runtime_error("Error: unable to create descriptor pool.");
    }
}

void HelloTriangleApplication::create_descriptor_sets()
{
    std::vector layouts(max_frames_in_flight_, descriptor_set_layout_);

    VkDescriptorSetAllocateInfo descriptor_set_allocate_info{};
    descriptor_set_allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptor_set_allocate_info.descriptorPool = descriptor_pool_;
    descriptor_set_allocate_info.descriptorSetCount = static_cast<uint32_t>(max_frames_in_flight_);
    descriptor_set_allocate_info.pSetLayouts = layouts.data();

    descriptor_sets_.resize(max_frames_in_flight_);
    if (vkAllocateDescriptorSets(device_, &descriptor_set_allocate_info, descriptor_sets_.data()) != VK_SUCCESS)
    {
        throw std::runtime_error("Error: unable to allocate descriptor sets.");
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
        descriptor_write.pImageInfo = nullptr;
        descriptor_write.pTexelBufferView = nullptr;

        vkUpdateDescriptorSets(device_, 1, &descriptor_write, 0, nullptr);
    }
}

void HelloTriangleApplication::create_command_buffers()
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

void HelloTriangleApplication::create_sync_objects()
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

void HelloTriangleApplication::record_command_buffer(VkCommandBuffer command_buffer, uint32_t image_index)
{
    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = 0;
    begin_info.pInheritanceInfo = nullptr;

    if (vkBeginCommandBuffer(command_buffer, &begin_info) != VK_SUCCESS)
    {
        throw std::runtime_error("Error: unable to begin command buffer.");
    }

    VkRenderPassBeginInfo render_pass_begin_info{};
    render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_begin_info.renderPass = render_pass_;
    render_pass_begin_info.framebuffer = swap_chain_framebuffers_[image_index];
    render_pass_begin_info.renderArea.offset = {0, 0};
    render_pass_begin_info.renderArea.extent = swap_chain_extent_;
    // which color to use when clearing
    VkClearValue clear_color = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
    render_pass_begin_info.clearValueCount = 1;
    render_pass_begin_info.pClearValues = &clear_color;
    
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

    VkBuffer vertex_buffers[] = {vertex_buffer_};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(command_buffer, 0, 1, vertex_buffers,offsets);

    vkCmdBindIndexBuffer(command_buffer, index_buffer_, 0, VK_INDEX_TYPE_UINT16);

    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout_, 0, 1, &descriptor_sets_[current_frame_], 0, nullptr);

    vkCmdDrawIndexed(command_buffer, static_cast<uint32_t>(test_shape_indices.size()), 1, 0, 0, 0);

    vkCmdEndRenderPass(command_buffer);

    if (vkEndCommandBuffer(command_buffer) != VK_SUCCESS)
    {
        throw std::runtime_error("Error: unable to record command buffer.");
    }
}

void HelloTriangleApplication::main_loop()
{
    while (!glfwWindowShouldClose(window_))
    {
        glfwPollEvents();
        draw_frame();
    }

    vkDeviceWaitIdle(device_);
}

void HelloTriangleApplication::draw_frame()
{
    vkWaitForFences(device_, 1, &in_flight_fences_[current_frame_], VK_TRUE, UINT64_MAX);

    uint32_t image_index;
    auto result = vkAcquireNextImageKHR(device_, swap_chain_, UINT64_MAX, image_available_semaphores_[current_frame_], VK_NULL_HANDLE, &image_index);

    if (result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        recreate_swap_chain();
        return;
    }
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
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

void HelloTriangleApplication::update_uniform_buffer()
{
    static auto start_time = std::chrono::high_resolution_clock::now();

    auto current_time = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float>(current_time - start_time).count();

    UniformBufferObject ubo{};
    ubo.model = rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.view = lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.proj = glm::perspective(glm::radians(45.0f), static_cast<float>(swap_chain_extent_.width) / static_cast<float>(swap_chain_extent_.height), 0.1f, 10.0f);
    ubo.proj[1][1] *= -1;

    memcpy(uniform_buffers_mapped_[current_frame_], &ubo, sizeof(ubo));
}

void HelloTriangleApplication::clean_up()
{
    clean_up_swap_chain();

    for (size_t i = 0; i < max_frames_in_flight_; ++i)
    {
        vkDestroyBuffer(device_, uniform_buffers_[i], nullptr);
        vkFreeMemory(device_, uniform_buffers_memories_[i], nullptr);
    }

    vkDestroyDescriptorPool(device_, descriptor_pool_, nullptr);

    vkDestroyDescriptorSetLayout(device_, descriptor_set_layout_, nullptr);

    vkDestroyBuffer(device_, index_buffer_, nullptr);
    vkFreeMemory(device_, index_buffer_memory_, nullptr);

    vkDestroyBuffer(device_, vertex_buffer_, nullptr);
    vkFreeMemory(device_, vertex_buffer_memory_, nullptr);
    
    for (size_t i = 0; i < max_frames_in_flight_; ++i)
    {
        vkDestroySemaphore(device_, image_available_semaphores_[i], nullptr);
        vkDestroySemaphore(device_, render_finished_semaphores_[i], nullptr);
        vkDestroyFence(device_, in_flight_fences_[i], nullptr);
    }
    
    vkDestroyCommandPool(device_, command_pool_, nullptr);

    vkDestroyCommandPool(device_, transfer_command_pool_, nullptr);
    
    vkDestroyPipeline(device_, graphics_pipeline_, nullptr);
    
    vkDestroyPipelineLayout(device_, pipeline_layout_, nullptr);

    vkDestroyRenderPass(device_, render_pass_, nullptr);
    
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

void HelloTriangleApplication::clean_up_swap_chain()
{
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
