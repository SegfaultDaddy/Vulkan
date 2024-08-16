#include <stdexcept>

#include <iostream>
#include <print>
#include <ranges>
#include <set>
#include <algorithm>

#include "triangle.hpp"

#undef max

namespace app
{   
    Triangle::Triangle(const std::uint32_t width, const std::uint32_t height)
        : physicalDevice{VK_NULL_HANDLE}
    {
        create_window(width, height, name);
        create_instance();
        //show_extensions_support();
        setup_debug_messages();
        create_surface();
        pick_physical_device();
        create_logical_device();
        create_swap_chain();
    }
    
    Triangle::~Triangle()
    {
        vkDestroyDevice(device, nullptr);
        if(enableValidationLayers)
        {
            destroy_debug_utils_messenger_ext(instance, debugMessenger, nullptr);
        }

        vkDestroySurfaceKHR(instance, surface, nullptr);
        vkDestroyInstance(instance, nullptr);

        glfwDestroyWindow(window);
        glfwTerminate();
    }

    void Triangle::run()
    {
        while(!glfwWindowShouldClose(window))
        {
            glfwPollEvents();
        }
    }  

    void Triangle::create_window(const std::uint32_t width, const std::uint32_t height, const std::string_view name)
    {
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        window = glfwCreateWindow(width, height, std::data(name), nullptr, nullptr);
    }

    void Triangle::create_instance()
    {
        if(enableValidationLayers && !check_validation_layer_support())
        {
            throw std::runtime_error{"Error: validation layers requested, but not available!"};
        }

        VkApplicationInfo info{};
        info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        info.pApplicationName = std::data(name);
        info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        info.pEngineName = "No Engine";
        info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        info.apiVersion = VK_API_VERSION_1_0;
        info.pNext = nullptr;

        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &info;

        auto extensions{required_extensions()};

        createInfo.enabledExtensionCount = static_cast<std::uint32_t>(std::size(extensions));
        createInfo.ppEnabledExtensionNames = std::data(extensions);

        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
        if(enableValidationLayers)
        {
            createInfo.enabledLayerCount = static_cast<std::uint32_t>(std::size(validationLayers));
            createInfo.ppEnabledLayerNames = std::data(validationLayers);

            populate_debug_messenger_create_info(debugCreateInfo);
            createInfo.pNext = static_cast<VkDebugUtilsMessengerCreateInfoEXT*>(&debugCreateInfo);
        }
        else
        {
            createInfo.enabledLayerCount = 0;
            createInfo.pNext = nullptr;
        }

        if(VkResult result{vkCreateInstance(&createInfo, nullptr, &instance)}; 
           result != VK_SUCCESS)
        {
            throw std::runtime_error{"Error: failed to create instance."};
        }
    }

    void Triangle::show_extensions_support() const
    {
        std::uint32_t extensionCount{0};
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

        std::vector<VkExtensionProperties> extensions(extensionCount);
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, std::data(extensions));
        
        std::println("Available extensions:");
        for(const auto& extension : extensions)
        {
            std::println("\t{}", extension.extensionName);
        }
    }

    bool Triangle::check_validation_layer_support() const
    {
        std::uint32_t layersCount{0};
        vkEnumerateInstanceLayerProperties(&layersCount, nullptr);

        std::vector<VkLayerProperties> availableLayers(layersCount);
        vkEnumerateInstanceLayerProperties(&layersCount, std::data(availableLayers));
        
        for(const auto& layerName : validationLayers)
        {
            bool layerFound{false};
            for(const auto& layerProperties : availableLayers)
            {
                if(std::string_view{layerName} == layerProperties.layerName)
                {
                    layerFound = true;
                    break;
                }
            }

            if(!layerFound)
            {
                return false;
            }
        }
        return true;
    }

    std::vector<const char*> Triangle::required_extensions() const
    {
        std::uint32_t glfwExtensionCount{0};
        const char** glfwExtensions{glfwGetRequiredInstanceExtensions(&glfwExtensionCount)};

        std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
        if(enableValidationLayers)
        {
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        return extensions;
    }

    VKAPI_ATTR VkBool32 VKAPI_CALL Triangle::debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, 
                                                            VkDebugUtilsMessageSeverityFlagsEXT messageType,
                                                            const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
                                                            void* userData)
    {
        std::println(std::cerr, "Validation layer: {}", callbackData->pMessage);
        if(messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
        {
        }
        return VK_FALSE;
    }

    void Triangle::setup_debug_messages()
    {
        if(!enableValidationLayers)
        {
            return;
        }

        VkDebugUtilsMessengerCreateInfoEXT createInfo{};
        populate_debug_messenger_create_info(createInfo);

        if(create_debug_utils_messanger_ext(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS)
        {
            throw std::runtime_error{"Error: failed to set up debug messanger."};
        }
    }

    void Triangle::populate_debug_messenger_create_info(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
    {
        createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity =VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | 
                                    VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                    VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = &debug_callback;
        createInfo.pUserData = nullptr;
    }

    void Triangle::pick_physical_device()
    {
        std::uint32_t deviceCount{0};
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

        if(deviceCount == 0)
        {
            throw std::runtime_error{"failed to find GPUs with Vulkan support"};
        }

        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, std::data(devices));

        for(const auto& device : devices)
        {
            if(is_device_suitable(device))
            {
                physicalDevice = device;
                break;
            }
        }

        if(physicalDevice == VK_NULL_HANDLE)
        {
            throw std::runtime_error{"failed to find a suitable GPU"};
        }
    }

    bool Triangle::check_device_extension_support(VkPhysicalDevice device)
    {
        std::uint32_t extensionCount{0};
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, std::data(availableExtensions));

        std::set<std::string_view> requiredExtensions(std::begin(deviceExtensions), std::end(deviceExtensions));
        for(const auto& extensions : availableExtensions)
        {
            requiredExtensions.erase(extensions.extensionName);
        }
        return std::empty(requiredExtensions);
    }

    bool Triangle::is_device_suitable(VkPhysicalDevice device)
    {
        VkPhysicalDeviceProperties deviceProperties{};
        vkGetPhysicalDeviceProperties(device, &deviceProperties);

        VkPhysicalDeviceFeatures deviceFeatures{};
        vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

        auto extensionsSupported{check_device_extension_support(device)};

        bool swapChainAdequate{false};

        if(extensionsSupported)
        {
            auto swapChainDetails{query_swap_chain_support(device)};
            swapChainAdequate = !std::empty(swapChainDetails.formats) && !std::empty(swapChainDetails.presentModes);
        }
        
        return deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
               deviceFeatures.geometryShader &&
               find_queue_families(device).is_complete() &&
               extensionsSupported &&
               swapChainAdequate;
    }

    QueueFamilyIndices Triangle::find_queue_families(VkPhysicalDevice device)
    {
        QueueFamilyIndices indices{};

        std::uint32_t queueFamilyCount{0};
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, std::data(queueFamilies));

        for(const auto& [i, queueFamily] : queueFamilies | std::views::enumerate)
        {
            if(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                indices.graphicsFamily = i;
            }

            VkBool32 presentSupport{false};
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

            if(presentSupport)
            {
                indices.presentFamily = i;
            }

            if(indices.is_complete())
            {
                break;
            }
        }
        return indices;
    }

    void Triangle::create_logical_device()
    {
        auto indices{find_queue_families(physicalDevice)};

        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos{};

        std::vector<std::uint32_t> uniqueQueueFamilies{indices.graphicsFamily.value(), indices.presentFamily.value()};

        float queuePriority{1.0f};
        for(const auto queueFamily : uniqueQueueFamilies)
        {
            VkDeviceQueueCreateInfo queueCreateInfo{};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;

            queueCreateInfos.push_back(std::move(queueCreateInfo));
        }

        VkPhysicalDeviceFeatures deviceFeatures{};

        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.queueCreateInfoCount = std::size(queueCreateInfos);
        createInfo.pQueueCreateInfos = std::data(queueCreateInfos);
        createInfo.pEnabledFeatures = &deviceFeatures;
        createInfo.enabledExtensionCount = static_cast<std::uint32_t>(std::size(deviceExtensions));
        createInfo.ppEnabledExtensionNames = std::data(deviceExtensions);

        if(enableValidationLayers)
        {
            createInfo.enabledLayerCount = static_cast<std::uint32_t>(std::size(validationLayers));
            createInfo.ppEnabledLayerNames = std::data(validationLayers);
        }
        else
        {
            createInfo.enabledLayerCount = std::size(validationLayers);
        }

        if(vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS)
        {
            throw std::runtime_error{"Error: failed to create logical device."};
        }

        vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
        vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
    }

    void Triangle::create_surface()
    {
        if(glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
        {
            throw std::runtime_error{"Error: failed to create window surface."};
        }
    }

    VkSurfaceFormatKHR Triangle::choose_swap_surface_format(const std::vector<VkSurfaceFormatKHR>& availableFormats) const
    {
        for(const auto& availableFormat : availableFormats)
        {
            if(availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && 
               availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            {
                return availableFormat;
            }
        }
        return availableFormats[0];
    }
    
    VkPresentModeKHR Triangle::choose_swap_present_mode(const std::vector<VkPresentModeKHR>& availablePresentModes) const
    {
        for(const auto& availablePresentMode : availablePresentModes)
        {
            if(availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
            {
                return availablePresentMode;
            }
        }

        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D Triangle::choose_swap_extent(const VkSurfaceCapabilitiesKHR& capabilities) const
    {
        if(capabilities.currentExtent.width != std::numeric_limits<std::uint32_t>::max())
        {
            return capabilities.currentExtent;
        }

        struct 
        {
            std::int32_t width;
            std::int32_t height;
        } size;
        glfwGetFramebufferSize(window, &size.width, &size.height);

        VkExtent2D actualExtent{static_cast<std::uint32_t>(size.width), 
                                static_cast<std::uint32_t>(size.height)};
        actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actualExtent.width = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
        return actualExtent;
    }

    SwapChainSupportDetails Triangle::query_swap_chain_support(VkPhysicalDevice device)
    {
        SwapChainSupportDetails details{};
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

        std::uint32_t formatCount{0};
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

        if(formatCount != 0)
        {
            details.formats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, std::data(details.formats));
        }

        std::uint32_t presentModeCount{0};
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

        if(presentModeCount != 0)
        {
            details.presentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, std::data(details.presentModes));
        }

        return details;
    }

    void Triangle::create_swap_chain()
    {
        auto swapChainSupport{query_swap_chain_support(physicalDevice)};

        auto surfaceFormat{choose_swap_surface_format(swapChainSupport.formats)};
        auto presentMode{choose_swap_present_mode(swapChainSupport.presentModes)};
        auto extent{choose_swap_extent(swapChainSupport.capabilities)};
    }
} 