#include <stdexcept>

#include <vector>
#include <print>

#include "triangle.hpp"

namespace app
{
    Triangle::Triangle(const std::uint32_t width, const std::uint32_t height, const std::string_view name)
    {
        create_window(width, height, name);

        create_instance();
        show_extensions_support();
    }
    
    Triangle::~Triangle()
    {
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
        VkApplicationInfo info{};
        info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        info.pApplicationName = "Vulkan";
        info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        info.pEngineName = "No Engine";
        info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        info.apiVersion = VK_API_VERSION_1_0;
        info.pNext = nullptr;
        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &info;

        std::uint32_t glfwExtensionCount{0};
        const char** glfwExtensions{glfwGetRequiredInstanceExtensions(&glfwExtensionCount)};

        createInfo.enabledExtensionCount = glfwExtensionCount;
        createInfo.ppEnabledExtensionNames = glfwExtensions;
        createInfo.enabledLayerCount = 0;

        if(VkResult result{vkCreateInstance(&createInfo, nullptr, &instance)}; 
           result != VK_SUCCESS)
        {
            throw std::runtime_error{"Error: failed to create instance."};
        }
    }

    void Triangle::show_extensions_support()
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
} 