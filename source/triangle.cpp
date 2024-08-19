#include <stdexcept>
#include <iostream>
#include <print>
#include <ranges>
#include <set>
#include <algorithm>
#include <chrono>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "stb_image.hpp"

#include "file.hpp"
#include "triangle.hpp"

#undef max

namespace app
{   
    Triangle::Triangle(const std::uint32_t width, const std::uint32_t height)
        : physicalDevice{VK_NULL_HANDLE}, currentFrame{0}
        , framebufferResized{false}
    {
        create_window(width, height, name);
        create_instance();
        show_extensions_support();
        setup_debug_messages();
        create_surface();
        pick_physical_device();
        create_logical_device();
        create_swap_chain();
        create_image_views();
        create_render_pass();
        create_descriptor_set_layout();
        create_graphics_pipeline();
        create_frame_buffers();
        create_command_pool();
        create_texture_image();
        create_texture_image_view();
        create_texture_sampler();
        create_vertex_buffer();
        create_index_buffer();
        create_uniform_buffers();
        create_descriptor_pool();
        create_descriptor_sets();
        create_command_buffers();
        create_sync_objects();
    }
    
    Triangle::~Triangle()
    {
        cleanup_swap_chain();

        vkDestroySampler(device, textureSampler, nullptr);
        vkDestroyImageView(device, textureImageView, nullptr);

        vkDestroyImage(device, textureImage, nullptr);
        vkFreeMemory(device, textureImageMemory, nullptr);
        
        for(const auto i : std::views::iota(0, maxFramesInFlight))
        {
            vkDestroyBuffer(device, uniformBuffers[i], nullptr);
            vkFreeMemory(device, uniformBuffersMemory[i], nullptr);
        }

        vkDestroyDescriptorPool(device, descriptorPool, nullptr);
        vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

        vkDestroyBuffer(device, indexBuffer, nullptr);
        vkFreeMemory(device, indexBufferMemory, nullptr);

        vkDestroyBuffer(device, vertexBuffer, nullptr);
        vkFreeMemory(device, vertexBufferMemory, nullptr);

        for(const auto i : std::views::iota(0, maxFramesInFlight))
        {
            vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
            vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
            vkDestroyFence(device, inFlightFences[i], nullptr);
        }

        vkDestroyPipeline(device, graphicsPipeline, nullptr);
        vkDestroyPipelineLayout(device, pipelineLayout, nullptr);

        vkDestroyRenderPass(device, renderPass, nullptr);

        vkDestroyCommandPool(device, commandPool, nullptr);
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
            draw_frame();
        }

        vkDeviceWaitIdle(device);
    }  

    void Triangle::create_window(const std::uint32_t width, const std::uint32_t height, const std::string_view name)
    {
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        //glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        window = glfwCreateWindow(width, height, std::data(name), nullptr, nullptr);
        glfwSetWindowUserPointer(window, this);
        glfwSetFramebufferSizeCallback(window, &framebuffer_resize_callback);
        
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
        std::println(std::cerr, "{}", callbackData->pMessage);
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

        VkPhysicalDeviceFeatures supportedFeatures{};
        vkGetPhysicalDeviceFeatures(device, &supportedFeatures);
        
        return deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
               deviceFeatures.geometryShader &&
               find_queue_families(device).is_complete() &&
               extensionsSupported &&
               swapChainAdequate &&
               supportedFeatures.samplerAnisotropy;
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
        deviceFeatures.samplerAnisotropy = VK_TRUE;

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

        auto imageCount{swapChainSupport.capabilities.minImageCount + 1};
        if(swapChainSupport.capabilities.maxImageCount > 0 &&
           imageCount > swapChainSupport.capabilities.maxImageCount)
        {
            imageCount = swapChainSupport.capabilities.maxImageCount;
        }

        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = surface;
        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        auto indices{find_queue_families(physicalDevice)};
        std::vector<std::uint32_t> queueFamilyIndices{indices.graphicsFamily.value(), indices.presentFamily.value()};

        if(indices.graphicsFamily != indices.presentFamily)
        {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = std::data(queueFamilyIndices);
        }
        else
        {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            createInfo.queueFamilyIndexCount = 0;
            createInfo.pQueueFamilyIndices = nullptr;
        }
        createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;
        createInfo.oldSwapchain = VK_NULL_HANDLE;

        if(vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS)
        {
            throw std::runtime_error{"Error: failed to create swap chain."};
        }

        vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
        swapChainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(device, swapChain, &imageCount, std::data(swapChainImages));

        swapChainImageFormat = surfaceFormat.format;
        swapChainExtent = extent;
    }

    void Triangle::cleanup_swap_chain()
    {
        for(auto framebuffer : swapChainFrameBuffers)
        {
            vkDestroyFramebuffer(device, framebuffer, nullptr);
        }

        for(auto imageView : swapChainImageViews)
        {
            vkDestroyImageView(device, imageView, nullptr);
        }

        vkDestroySwapchainKHR(device, swapChain, nullptr);
    }

    void Triangle::recreate_swap_chain()
    {
        struct         
        {
            std::int32_t width;
            std::int32_t height;
        } size;

        glfwGetFramebufferSize(window, &size.width, &size.height);
        while(size.width == 0 || size.height ==0)
        {
            glfwGetFramebufferSize(window, &size.width, &size.height);
            glfwWaitEvents();
        }
        
        vkDeviceWaitIdle(device);

        cleanup_swap_chain();

        create_swap_chain();
        create_image_views();
        create_frame_buffers();
    }

    void Triangle::create_image_views()
    {
        swapChainImageViews.resize(std::size(swapChainImages));

        for(const auto i : std::views::iota(0ull, std::size(swapChainImages)))
        {
            swapChainImageViews[i] = create_image_view(swapChainImages[i], swapChainImageFormat);
        }
    }

    void Triangle::create_descriptor_set_layout()
    {
        VkDescriptorSetLayoutBinding uboLayoutBinding{};
        uboLayoutBinding.binding = 0;
        uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboLayoutBinding.descriptorCount = 1;
        uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        uboLayoutBinding.pImmutableSamplers = nullptr;

        VkDescriptorSetLayoutBinding samplerLayoutBinding{};
        samplerLayoutBinding.binding = 1;
        samplerLayoutBinding.descriptorCount = 1;
        samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        samplerLayoutBinding.pImmutableSamplers = nullptr;
        samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        std::array<VkDescriptorSetLayoutBinding, 2> bindings{uboLayoutBinding, samplerLayoutBinding};
        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<std::uint32_t>(std::size(bindings));
        layoutInfo.pBindings = std::data(bindings);

        if(vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS)
        {
            throw std::runtime_error{"Error: failed to create descriptor set layout."};
        }
    }

    void Triangle::create_graphics_pipeline()
    {
        auto vertexShaderCode{file::read_file("../shader/vert.spv")};
        auto fragmentShaderCode{file::read_file("../shader/frag.spv")};

        auto vertexShaderModule{create_shader_module(vertexShaderCode)};
        auto fragmentShaderModule{create_shader_module(fragmentShaderCode)};

        VkPipelineShaderStageCreateInfo vertexShaderCreateInfo{};
        vertexShaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertexShaderCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertexShaderCreateInfo.module = vertexShaderModule;
        vertexShaderCreateInfo.pName = "main";

        VkPipelineShaderStageCreateInfo fragmentShaderCreateInfo{};
        fragmentShaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragmentShaderCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragmentShaderCreateInfo.module = fragmentShaderModule;
        fragmentShaderCreateInfo.pName = "main";

        std::vector<VkPipelineShaderStageCreateInfo> shaderStages{vertexShaderCreateInfo, fragmentShaderCreateInfo};

        std::vector<VkDynamicState> dynamicStates{VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = static_cast<std::uint32_t>(std::size(dynamicStates));
        dynamicState.pDynamicStates = std::data(dynamicStates);

        auto bindingDescription{Vertex::binding_description()};
        auto attributeDescription{Vertex::attribute_description()};

        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<std::uint32_t>(std::size(attributeDescription));
        vertexInputInfo.pVertexAttributeDescriptions = std::data(attributeDescription);

        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        VkViewport viewport{};
        viewport.x = 0.0f; 
        viewport.y = 0.0f; 
        viewport.width = static_cast<float>(swapChainExtent.width); 
        viewport.height = static_cast<float>(swapChainExtent.height); 
        viewport.maxDepth = 0.0f; 
        viewport.maxDepth = 1.0f; 

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = swapChainExtent;

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.pViewports = &viewport;
        viewportState.scissorCount = 1;
        viewportState.pScissors = &scissor;

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

        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | 
                                              VK_COLOR_COMPONENT_G_BIT | 
                                              VK_COLOR_COMPONENT_B_BIT | 
                                              VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE;
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.logicOp = VK_LOGIC_OP_COPY;
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;
        colorBlending.blendConstants[0] = 0.0f;
        colorBlending.blendConstants[1] = 0.0f;
        colorBlending.blendConstants[2] = 0.0f;
        colorBlending.blendConstants[3] = 0.0f;

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
        pipelineLayoutInfo.pushConstantRangeCount = 0;
        pipelineLayoutInfo.pPushConstantRanges = nullptr;

        if(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
        {
            throw std::runtime_error{"Error: failed to create pipeline."};
        }

        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = std::data(shaderStages);
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = nullptr;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.layout = pipelineLayout;
        pipelineInfo.renderPass = renderPass;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
        pipelineInfo.basePipelineIndex = -1;

        if(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS)
        {
            throw std::runtime_error{"Error: failed to create graphics pipeline."};
        }

        vkDestroyShaderModule(device, vertexShaderModule, nullptr);
        vkDestroyShaderModule(device, fragmentShaderModule, nullptr);
    }

    VkShaderModule Triangle::create_shader_module(const std::vector<char>& code)
    {
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = std::size(code);
        createInfo.pCode = reinterpret_cast<const std::uint32_t*>(std::data(code));

        VkShaderModule shaderModule{};

        if(vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
        {
            throw std::runtime_error{"Error: failed to create shader module."};
        }

        return shaderModule;
    }

    void Triangle::create_render_pass()
    {
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = swapChainImageFormat;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;

        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;

        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = 1;
        renderPassInfo.pAttachments = &colorAttachment;
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        if(vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS)
        {
            throw std::runtime_error{"Error: failed to create render pass."};
        }
    }

    void Triangle::create_frame_buffers()
    {
        swapChainFrameBuffers.resize(std::size(swapChainImageViews));
        for(const auto i : std::views::iota(0ull, std::size(swapChainImageViews)))
        {
            std::vector<VkImageView> attachments{swapChainImageViews[i]};

            VkFramebufferCreateInfo frameBufferInfo{};
            frameBufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            frameBufferInfo.renderPass = renderPass;
            frameBufferInfo.attachmentCount = 1;
            frameBufferInfo.pAttachments = std::data(attachments);
            frameBufferInfo.width = swapChainExtent.width;
            frameBufferInfo.height = swapChainExtent.height;
            frameBufferInfo.layers = 1;

            if(vkCreateFramebuffer(device, &frameBufferInfo, nullptr, &swapChainFrameBuffers[i]) != VK_SUCCESS)
            {
                throw std::runtime_error{"Error: failed to create framebuffer."};
            }
        }
    }

    void Triangle::create_command_pool()
    {
        auto queueFamilyIndices{find_queue_families(physicalDevice)};

        VkCommandPoolCreateInfo commandPoolCreateInfo{};
        commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        commandPoolCreateInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

        if(vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, &commandPool))
        {
            throw std::runtime_error{"Error: failed to create command pool."};
        }
    }
    
    void Triangle::create_image(std::uint32_t width, std::uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, 
                                VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory)
    {
        VkImageCreateInfo imageCreateInfo{};
        imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
        imageCreateInfo.extent.width = static_cast<std::uint32_t>(width);
        imageCreateInfo.extent.height = static_cast<std::uint32_t>(height);
        imageCreateInfo.extent.depth = 1;
        imageCreateInfo.mipLevels = 1;
        imageCreateInfo.arrayLayers = 1;
        imageCreateInfo.format = format;
        imageCreateInfo.tiling = tiling;
        imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageCreateInfo.usage = usage;
        imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageCreateInfo.flags = 0;

        if(vkCreateImage(device, &imageCreateInfo, nullptr, &image) != VK_SUCCESS)
        {
            throw std::runtime_error{"Error: failed to create image."};
        }

        VkMemoryRequirements memoryRequirements{};
        vkGetImageMemoryRequirements(device, image, &memoryRequirements);

        VkMemoryAllocateInfo allocateInfo{};
        allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocateInfo.allocationSize = memoryRequirements.size;
        allocateInfo.memoryTypeIndex = find_memory_type(memoryRequirements.memoryTypeBits, properties);

        if(vkAllocateMemory(device, &allocateInfo, nullptr, &imageMemory) != VK_SUCCESS)
        {
            throw std::runtime_error{"Error: failed to allocate image memory."};
        }

        vkBindImageMemory(device, image, imageMemory, 0);
    }

    void Triangle::create_texture_image()
    {
        struct 
        {
            std::int32_t  width;
            std::int32_t  height;
            std::int32_t  channels;
        } texture;
        stbi_uc* pixels = stbi_load("../texture/texture.jpg", &texture.width, &texture.height, &texture.channels, STBI_rgb_alpha);
        VkDeviceSize imageSize{static_cast<VkDeviceSize>(texture.width * texture.height * 4)};

        if(pixels == nullptr)
        {
            throw std::runtime_error{"Error: failed to load texture image."};
        }

        VkBuffer stagingBuffer{};
        VkDeviceMemory stagingBufferMemory{};
        
        create_buffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | 
                      VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

        void* data{};
        vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
        memcpy(data, pixels, static_cast<std::size_t>(imageSize));
        vkUnmapMemory(device, stagingBufferMemory);

        stbi_image_free(pixels);

        create_image(texture.width, texture.height, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | 
                     VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage, textureImageMemory);
        transition_image_layout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        copy_buffer_to_image(stagingBuffer, textureImage, static_cast<std::uint32_t>(texture.width), static_cast<std::uint32_t>(texture.height));
        transition_image_layout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingBufferMemory, nullptr);
    }
    
    VkImageView Triangle::create_image_view(VkImage image, VkFormat format)
    {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = format;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        VkImageView imageView{};
        if(vkCreateImageView(device, &viewInfo, nullptr, &imageView) != VK_SUCCESS)
        {
            throw std::runtime_error{"Error: failed to create texture image view."};
        }

        return imageView;
    }

    void Triangle::create_texture_image_view()
    {
        textureImageView = create_image_view(textureImage, VK_FORMAT_R8G8B8A8_SRGB);
    }
    
    void Triangle::create_texture_sampler()
    {
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.anisotropyEnable = VK_FALSE;
        samplerInfo.maxAnisotropy = 1.0f;

        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(physicalDevice, &properties);

        samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy; 
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.mipLodBias = 0.0f;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = 0.0f;

        if(vkCreateSampler(device, &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS)
        {
            throw std::runtime_error{"Error: failed to create texture sampler."};
        }
    }

    void Triangle::create_buffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, 
                                 VkBuffer& buffer, VkDeviceMemory& bufferMemory)
    {
        VkBufferCreateInfo bufferCreateInfo{};
        bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferCreateInfo.size = size;
        bufferCreateInfo.usage = usage;
        bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if(vkCreateBuffer(device, &bufferCreateInfo, nullptr, &buffer) != VK_SUCCESS)
        {
            throw std::runtime_error{"Error: failed to create vertex buffer."};
        }

        VkMemoryRequirements memoryRequirements{};
        vkGetBufferMemoryRequirements(device, buffer, &memoryRequirements);

        VkMemoryAllocateInfo allocateInfo{};
        allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocateInfo.allocationSize = memoryRequirements.size;
        allocateInfo.memoryTypeIndex = find_memory_type(memoryRequirements.memoryTypeBits, properties);
        
        if(vkAllocateMemory(device, &allocateInfo, nullptr, &bufferMemory) != VK_SUCCESS)
        {
            throw std::runtime_error{"Error: failed to allocate vertex buffer memory."};
        }

        vkBindBufferMemory(device, buffer, bufferMemory, 0);
    }

    void Triangle::create_vertex_buffer()
    {
        VkDeviceSize bufferSize{sizeof(vertices[0]) * std::size(vertices)};

        VkBuffer stagingBuffer{};
        VkDeviceMemory stagingBufferMemory{};
        create_buffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | 
                      VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

        void* data{nullptr};
        vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, std::data(vertices), static_cast<std::size_t>(bufferSize));
        vkUnmapMemory(device, stagingBufferMemory);

        create_buffer(bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, 
                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);

        copy_buffer(stagingBuffer, vertexBuffer, bufferSize);

        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingBufferMemory, nullptr);
    }

    void Triangle::create_index_buffer()
    {
        VkDeviceSize bufferSize{sizeof(indices[0]) * std::size(indices)};

        VkBuffer stagingBuffer{};
        VkDeviceMemory stagingBufferMemory{};
        create_buffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | 
                      VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

        void* data{};
        vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, std::data(indices), static_cast<std::size_t>(bufferSize));
        vkUnmapMemory(device, stagingBufferMemory);

        create_buffer(bufferSize,VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory);

        copy_buffer(stagingBuffer, indexBuffer, bufferSize);

        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingBufferMemory, nullptr);
    }

    std::uint32_t Triangle::find_memory_type(std::uint32_t typeFilter, VkMemoryPropertyFlags properties)
    {
        VkPhysicalDeviceMemoryProperties memoryProperties{};
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);
        for(const auto i : std::views::iota(0u, memoryProperties.memoryTypeCount))
        {
            if(typeFilter & (1 << i) && 
              (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
            {
                return i;
            }
        }

        throw std::runtime_error{"Error: failed to find suitable memory type."};

        return {};
    }

    void Triangle::copy_buffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
    {
        VkCommandBuffer commandBuffer{begin_single_time_commands()};

        VkBufferCopy copyRegion{};
        copyRegion.srcOffset = 0;
        copyRegion.dstOffset = 0;
        copyRegion.size = size;
        vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

        end_single_time_commands(commandBuffer);
    }

    void Triangle::create_uniform_buffers()
    {
        VkDeviceSize bufferSize{sizeof(UniformBufferObject)};

        uniformBuffers.resize(maxFramesInFlight);
        uniformBuffersMemory.resize(maxFramesInFlight);
        uniformBuffersMapped.resize(maxFramesInFlight);

        for(const auto i : std::views::iota(0, maxFramesInFlight))
        {
            create_buffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | 
                          VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffers[i], uniformBuffersMemory[i]);
            vkMapMemory(device, uniformBuffersMemory[i], 0, bufferSize, 0, &uniformBuffersMapped[i]);
        }
    }

    void Triangle::create_descriptor_pool()
    {
        std::array<VkDescriptorPoolSize, 2> poolSizes{};
        poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes[0].descriptorCount = static_cast<std::uint32_t>(maxFramesInFlight);
        poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSizes[1].descriptorCount = static_cast<std::uint32_t>(maxFramesInFlight);


        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = static_cast<std::uint32_t>(std::size(poolSizes));
        poolInfo.pPoolSizes = std::data(poolSizes);
        poolInfo.maxSets = static_cast<std::uint32_t>(maxFramesInFlight);

        if(vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS)
        {
            throw std::runtime_error{"Error: failed to create descriptor pool"};
        }
    }

    void Triangle::create_descriptor_sets()
    {
        std::vector<VkDescriptorSetLayout> layouts(maxFramesInFlight, descriptorSetLayout);
        VkDescriptorSetAllocateInfo allocateInfo{};
        allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocateInfo.descriptorPool = descriptorPool;
        allocateInfo.descriptorSetCount = static_cast<std::uint32_t>(maxFramesInFlight);
        allocateInfo.pSetLayouts = std::data(layouts);

        descriptorSets.resize(maxFramesInFlight);
        if(vkAllocateDescriptorSets(device, &allocateInfo, std::data(descriptorSets)) != VK_SUCCESS)
        {
            throw std::runtime_error{"Error: failed to allocate descriptor sets."};
        }

        for(const auto i : std::views::iota(0, maxFramesInFlight))
        {
            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = uniformBuffers[i];
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(UniformBufferObject);

            VkDescriptorImageInfo imageInfo{};
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo.imageView = textureImageView;
            imageInfo.sampler = textureSampler;

            std::array<VkWriteDescriptorSet, 2> descriptorWrites{};
            descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[0].dstSet = descriptorSets[i];
            descriptorWrites[0].dstBinding = 0;
            descriptorWrites[0].dstArrayElement = 0;
            descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrites[0].descriptorCount = 1;
            descriptorWrites[0].pBufferInfo = &bufferInfo;

            descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[1].dstSet = descriptorSets[i];
            descriptorWrites[1].dstBinding = 1;
            descriptorWrites[1].dstArrayElement = 0;
            descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptorWrites[1].descriptorCount = 1;
            descriptorWrites[1].pImageInfo = &imageInfo;

            vkUpdateDescriptorSets(device, static_cast<std::uint32_t>(std::size(descriptorWrites)), std::data(descriptorWrites), 0, nullptr);
        }
    }

    VkCommandBuffer Triangle::begin_single_time_commands()
    {
        VkCommandBufferAllocateInfo allocateInfo{};
        allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocateInfo.commandPool = commandPool;
        allocateInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer{};
        vkAllocateCommandBuffers(device, &allocateInfo, &commandBuffer);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(commandBuffer, &beginInfo);
        return commandBuffer;
    }

    void Triangle::end_single_time_commands(VkCommandBuffer commandBuffer)
    {
        vkEndCommandBuffer(commandBuffer);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(graphicsQueue);

        vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
    }

    void Triangle::transition_image_layout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout)
    {
        VkCommandBuffer commandBuffer{begin_single_time_commands()};

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = 0;

        VkPipelineStageFlags sourceStage{};
        VkPipelineStageFlags destinationStage{};

        if(oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
        {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        }
        else if(oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
        {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

            sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }
        else
        {
            throw std::invalid_argument{"Error: unsupported layout transition."};
        }

        vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 
                             0, nullptr, 1, &barrier);

        end_single_time_commands(commandBuffer);
    }

    void Triangle::copy_buffer_to_image(VkBuffer buffer, VkImage image, std::uint32_t width, std::uint32_t height)
    {
        VkCommandBuffer commandBuffer{begin_single_time_commands()};

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
        
        vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        end_single_time_commands(commandBuffer);
    }

    void Triangle::create_command_buffers()
    {
        commandBuffers.resize(maxFramesInFlight);

        VkCommandBufferAllocateInfo allocateInfo{};
        allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocateInfo.commandPool = commandPool;
        allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocateInfo.commandBufferCount = static_cast<std::uint32_t>(std::size(commandBuffers));

        if(vkAllocateCommandBuffers(device, &allocateInfo, std::data(commandBuffers)) != VK_SUCCESS)
        {
            throw std::runtime_error{"Error: failed to allocate command buffer."};
        }
    }

    void Triangle::record_command_buffer(VkCommandBuffer commandBuffer, std::uint32_t imageIndex)
    {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0;
        beginInfo.pInheritanceInfo = nullptr;

        if(vkBeginCommandBuffer(commandBuffer, &beginInfo))
        {
            throw std::runtime_error{"Error: failed to begin recordering command buffer."};
        }

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = renderPass;
        renderPassInfo.framebuffer = swapChainFrameBuffers[imageIndex];
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = swapChainExtent;

        VkClearValue clearColor{{{0.0f, 0.0f, 0.0f, 1.0f}}};
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColor;

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

        VkViewport viewport{};
        viewport.x = 0.0f; 
        viewport.y = 0.0f; 
        viewport.width = static_cast<float>(swapChainExtent.width); 
        viewport.height = static_cast<float>(swapChainExtent.height); 
        viewport.maxDepth = 0.0f; 
        viewport.maxDepth = 1.0f; 
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = swapChainExtent;
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        std::vector<VkBuffer> vertexBuffers{vertexBuffer};
        std::vector<VkDeviceSize> offsets{0};
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, std::data(vertexBuffers), std::data(offsets));

        vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT16);

        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 
                                0, 1, &descriptorSets[currentFrame], 0, nullptr);
        vkCmdDrawIndexed(commandBuffer, static_cast<std::uint32_t>(std::size(indices)), 1, 0, 0, 0);

        vkCmdEndRenderPass(commandBuffer);

        if(vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
        {
            throw std::runtime_error{"Error: failed to record a command buffer!"};
        }
    }

    void Triangle::create_sync_objects()
    {
        imageAvailableSemaphores.resize(maxFramesInFlight);
        renderFinishedSemaphores.resize(maxFramesInFlight);
        inFlightFences.resize(maxFramesInFlight);

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for(const auto i : std::views::iota(0, maxFramesInFlight))
        {
            if(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
               vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
               vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS)
            {
                throw std::runtime_error{"Error: failed to create semaphores!"};
            }
        }
    }

    void Triangle::draw_frame()
    {
        vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, std::numeric_limits<std::uint64_t>::max());

        std::uint32_t imageIndex{0};
        auto result{vkAcquireNextImageKHR(device, swapChain, std::numeric_limits<std::uint64_t>::max(), 
                                          imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex)};

        if(result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            recreate_swap_chain();
            return;
        }
        else if(result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
        {
            throw std::runtime_error{"Error: failed to acquire swap chain image."};
        }

        vkResetFences(device, 1, &inFlightFences[currentFrame]); 

        vkResetCommandBuffer(commandBuffers[currentFrame], 0);
        record_command_buffer(commandBuffers[currentFrame], imageIndex);

        update_uniform_buffer(currentFrame);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        std::vector<VkSemaphore> waitSemaphore{imageAvailableSemaphores[currentFrame]};
        std::vector<VkPipelineStageFlags> waitStages{VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = std::data(waitSemaphore);
        submitInfo.pWaitDstStageMask = std::data(waitStages);
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffers[currentFrame];

        std::vector<VkSemaphore> signalSemaphores{renderFinishedSemaphores[currentFrame]};
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = std::data(signalSemaphores);

        if(vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS)
        {
            throw std::runtime_error{"Error: failed to submit to queue."};
        }

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = std::data(signalSemaphores);

        std::vector<VkSwapchainKHR> swapChains{swapChain};

        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = std::data(swapChains);
        presentInfo.pImageIndices = &imageIndex;
        presentInfo.pResults = nullptr;

        result = vkQueuePresentKHR(presentQueue, &presentInfo);
        
        if(result == VK_ERROR_OUT_OF_DATE_KHR || 
           result == VK_SUBOPTIMAL_KHR ||
           framebufferResized)
        {
            framebufferResized = false;
            recreate_swap_chain();
        }
        else if(result != VK_SUCCESS)
        {
            throw std::runtime_error{"Error: failed to present swap chain image."};
        }

        currentFrame = (currentFrame + 1) % maxFramesInFlight;
    }

    void Triangle::update_uniform_buffer(std::uint32_t currentImage)
    {
        static auto startTime{std::chrono::high_resolution_clock::now()};

        auto currentTime{std::chrono::high_resolution_clock::now()};
        float time{std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count()};
        
        UniformBufferObject ubo{};
        ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.projection = glm::perspective(glm::radians(45.0f), swapChainExtent.width / static_cast<float>(swapChainExtent.height), 1.0f, 10.0f);
        ubo.projection[1][1] *= -1;
        memcpy(uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
    }

    void Triangle::framebuffer_resize_callback(GLFWwindow* window, std::int32_t width, std::int32_t height)
    {
        auto appliacation{reinterpret_cast<Triangle*>(glfwGetWindowUserPointer(window))};
        appliacation->framebufferResized = true;
    }
} 