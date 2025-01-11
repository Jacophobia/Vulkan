// ReSharper disable CppMemberFunctionMayBeConst
// ReSharper disable CppUseStructuredBinding

#include "HelloTriangleApplication.h"

#include <algorithm>
#include <stdexcept>
#include <vector>
#include <format>
#include <fstream>
#include <set>

#include "../Logging/Logging.h"

HelloTriangleApplication::HelloTriangleApplication() :
    window_(nullptr), instance_(), debug_messenger_(), device_(),
    graphics_queue_(), present_queue_(), surface_() {}

HelloTriangleApplication::~HelloTriangleApplication() = default;

void HelloTriangleApplication::run()
{
    init_window();
    init_vulkan();
    main_loop();
    clean_up();
}

void HelloTriangleApplication::init_window()
{
    // initialize a window
    glfwInit();
    // don't load opengl
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    // disable window resizing
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    
    window_ = glfwCreateWindow(width_, height_, title_, nullptr, nullptr);
    
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
    create_graphics_pipeline();
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
            indices.graphics_family = i;
            graphics_support = true;
        }

        VkBool32 present_support = false;

        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface_, &present_support);

        if (present_support)
        {
            indices.present_family = i;
        }

        if (graphics_support && present_support)
        {
            return indices;
        }

        i++;
    }

    return indices;
}

void HelloTriangleApplication::create_logical_device()
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

    if (!indices.graphics_family.has_value() || !indices.present_family.has_value())
    {
        throw std::runtime_error("Error: queue family unavailable.");
    }

    if (indices.graphics_family != indices.present_family)
    {
        const uint32_t queue_family_indices[] =
        {
            indices.graphics_family.value(),
            indices.present_family.value(),
        };
        
        create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        create_info.queueFamilyIndexCount = 2;
        create_info.pQueueFamilyIndices = queue_family_indices;
    }
    else
    {
        create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        create_info.queueFamilyIndexCount = 0;
        create_info.pQueueFamilyIndices = nullptr;
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

    if (vkCreateRenderPass(device_, &render_pass_create_info, nullptr, &render_pass_) != VK_SUCCESS)
    {
        throw std::runtime_error("Error: unable to create render pass.");
    }
        

    
}

void create_pipeline_test()
{
    
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
    vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_info.vertexBindingDescriptionCount = 0;
    vertex_input_info.pVertexBindingDescriptions = nullptr;
    vertex_input_info.vertexAttributeDescriptionCount = 0;
    vertex_input_info.pVertexAttributeDescriptions = nullptr;

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
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
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
    pipeline_layout_create_info.setLayoutCount = 0;
    pipeline_layout_create_info.pSetLayouts = nullptr;
    pipeline_layout_create_info.pushConstantRangeCount = 0;
    pipeline_layout_create_info.pPushConstantRanges = nullptr;

    if (!vkCreatePipelineLayout(device_, &pipeline_layout_create_info, nullptr, &pipeline_layout_))
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

void HelloTriangleApplication::main_loop()
{
    while (!glfwWindowShouldClose(window_))
    {
        glfwPollEvents();
    }
}

void HelloTriangleApplication::clean_up()
{
    vkDestroyPipeline(device_, graphics_pipeline_, nullptr);
    
    vkDestroyPipelineLayout(device_, pipeline_layout_, nullptr);

    vkDestroyRenderPass(device_, render_pass_, nullptr);
    
    for (const auto image_view : swap_chain_image_views_)
    {
        vkDestroyImageView(device_, image_view, nullptr);
    }
    
    vkDestroySwapchainKHR(device_, swap_chain_, nullptr);
    
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
