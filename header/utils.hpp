#ifndef UTILS_HPP
#define UTILS_HPP

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace app
{
    VkResult create_debug_utils_messanger_ext(VkInstance instance, 
                                              const VkDebugUtilsMessengerCreateInfoEXT* createInfo,
                                              const VkAllocationCallbacks* allocator,
                                              VkDebugUtilsMessengerEXT* debugMessanger);
    void destroy_debug_utils_messenger_ext(VkInstance instance, 
                                           VkDebugUtilsMessengerEXT debugMessanger,
                                           const VkAllocationCallbacks* allocator);
}

#endif