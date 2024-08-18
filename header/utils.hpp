#ifndef UTILS_HPP
#define UTILS_HPP

#include <optional>
#include <vector>
#include <array>

#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>

namespace app
{
    constexpr inline std::array<const char*, 1> validationLayers
    {
        "VK_LAYER_KHRONOS_validation"
    };

    constexpr inline std::array<const char*, 1> deviceExtensions
    {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    struct Vertex
    {
        static VkVertexInputBindingDescription binding_description();
        static std::array<VkVertexInputAttributeDescription, 2> attribute_description();

        glm::vec2 position;
        glm::vec3 color;
    };

    struct QueueFamilyIndices
    {
        inline bool is_complete() const
        {
            return graphicsFamily.has_value() && presentFamily.has_value();
        }

        std::optional<std::uint32_t> graphicsFamily;
        std::optional<std::uint32_t> presentFamily;
    };

    struct SwapChainSupportDetails
    {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };
    
    VkResult create_debug_utils_messanger_ext(VkInstance instance, 
                                              const VkDebugUtilsMessengerCreateInfoEXT* createInfo,
                                              const VkAllocationCallbacks* allocator,
                                              VkDebugUtilsMessengerEXT* debugMessanger);
    void destroy_debug_utils_messenger_ext(VkInstance instance, 
                                           VkDebugUtilsMessengerEXT debugMessanger,
                                           const VkAllocationCallbacks* allocator);
}

#endif