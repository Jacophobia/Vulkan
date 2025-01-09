// ReSharper disable CppMemberFunctionMayBeConst
// ReSharper disable CppUseStructuredBinding
#include "HelloTriangleApplication.h"

#include <stdexcept>
#include <vector>
#include <format>
#include <set>

#include "../Logging/Logging.h"

HelloTriangleApplication::HelloTriangleApplication()
    : window_(nullptr), instance_(), debug_messenger_(), device_(), graphics_queue_() {}

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

    if (!device_features.geometryShader)
    {
        return 0;
    }

    auto indices = find_queue_families(device);

    if (!indices.graphics_family.has_value())
    {
        return 0;
    }

    auto device_extensions_supported = are_device_extensions_supported(device);

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

void HelloTriangleApplication::main_loop()
{
    while (!glfwWindowShouldClose(window_))
    {
        glfwPollEvents();
    }
}

void HelloTriangleApplication::clean_up()
{
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
