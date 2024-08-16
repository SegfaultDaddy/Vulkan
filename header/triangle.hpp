#ifndef TRIANGLE_APP_HPP
#define TRIANGLE_APP_HPP

#include <cstdint>
#include <string_view>
#include <array>
#include <vector>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace app
{
    class Triangle
    {
    public:
        Triangle(const std::uint32_t width, const std::uint32_t height, const std::string_view name = "Vulkan");
        ~Triangle();
        void run();
    private:
        void create_instance();
        void create_window(const std::uint32_t width, const std::uint32_t height, const std::string_view name);
        void show_extensions_support() const;
        bool check_validation_layer_support() const;
        std::vector<const char*> required_extensions() const;
        static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, 
                                                             VkDebugUtilsMessageSeverityFlagsEXT messageType,
                                                             const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
                                                             void* userData);

        GLFWwindow* window;

        VkInstance instance;
        
        constexpr static std::array<const char*, 1> validationLayers
        {
            "VK_LAYER_KHRONOS_validation"
        };

        #ifdef NDEBUG
            constexpr static bool enableValidationLayers{false};
        #else 
            constexpr static bool enableValidationLayers{true};
        #endif
    };
}

#endif