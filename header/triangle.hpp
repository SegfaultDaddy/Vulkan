#ifndef TRIANGLE_APP_HPP
#define TRIANGLE_APP_HPP

#include <cstdint>
#include <string_view>

#include "utils.hpp"

namespace app
{
    class Triangle final
    {
    public:
        Triangle(const std::uint32_t width, const std::uint32_t height);
        ~Triangle();
        void run();
    private:
        void create_instance();
        void create_window(const std::uint32_t width, const std::uint32_t height, const std::string_view name);
        static void framebuffer_resize_callback(GLFWwindow* window, std::int32_t width, std::int32_t height);
        void show_extensions_support() const;

        bool check_validation_layer_support() const;
        std::vector<const char*> required_extensions() const;
        static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, 
                                                             VkDebugUtilsMessageSeverityFlagsEXT messageType,
                                                             const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
                                                             void* userData);
        void setup_debug_messages();
        void populate_debug_messenger_create_info(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

        void pick_physical_device();
        bool check_device_extension_support(VkPhysicalDevice device);
        bool is_device_suitable(VkPhysicalDevice device);
        SwapChainSupportDetails query_swap_chain_support(VkPhysicalDevice device);
        QueueFamilyIndices find_queue_families(VkPhysicalDevice device);

        void create_logical_device();

        void create_surface();
        VkSurfaceFormatKHR choose_swap_surface_format(const std::vector<VkSurfaceFormatKHR>& availableFormats) const; 
        VkPresentModeKHR choose_swap_present_mode(const std::vector<VkPresentModeKHR>& availablePresentModes) const; 
        VkExtent2D choose_swap_extent(const VkSurfaceCapabilitiesKHR& capabilities) const;

        void create_swap_chain();
        void cleanup_swap_chain();
        void recreate_swap_chain();
        
        void create_image_views();
        
        void create_graphics_pipeline();
        VkShaderModule create_shader_module(const std::vector<char>& code);

        void create_render_pass();

        void create_frame_buffer();

        void create_command_pool();
        void create_command_buffers();
        void record_command_buffer(VkCommandBuffer commandBuffer, std::uint32_t imageIndex);
        void create_sync_objects();

        void draw_frame();

        GLFWwindow* window;

        VkInstance instance;
        VkDebugUtilsMessengerEXT debugMessenger;
        VkSurfaceKHR surface;
        
        VkPhysicalDevice physicalDevice;
        VkDevice device;

        VkQueue graphicsQueue;
        VkQueue presentQueue;

        VkSwapchainKHR swapChain;
        std::vector<VkImage> swapChainImages;
        VkFormat swapChainImageFormat;
        VkExtent2D swapChainExtent;
        std::vector<VkImageView> swapChainImagesViews;
        VkRenderPass renderPass;
        VkPipelineLayout pipelineLayout;
        VkPipeline graphicsPipeline;
        std::vector<VkFramebuffer> swapChainFrameBuffers;

        VkCommandPool commandPool;
        std::vector<VkCommandBuffer> commandBuffers;

        std::vector<VkSemaphore> imageAvailableSemaphores;
        std::vector<VkSemaphore> renderFinishedSemaphores;
        std::vector<VkFence> inFlightFences;

        bool framebufferResized;

        std::uint32_t currentFrame;

        constexpr static std::string_view name{"Vulkan Triangle"};

        constexpr static std::int32_t maxFramesInFlight{2};

        constexpr static std::array<Vertex, 3> vertices
        {
            Vertex{{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
            Vertex{{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
            Vertex{{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}
        };

        #ifdef NDEBUG
            constexpr static bool enableValidationLayers{false};
        #else 
            constexpr static bool enableValidationLayers{true};
        #endif
    };
}

#endif