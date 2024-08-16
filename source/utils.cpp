#include <cstdint>

#include "utils.hpp"

namespace app
{
    VkResult create_debug_utils_messanger_ext(VkInstance instance, 
                                              const VkDebugUtilsMessengerCreateInfoEXT* createInfo,
                                              const VkAllocationCallbacks* allocator,
                                              VkDebugUtilsMessengerEXT* debugMessanger)
    {
        auto function{reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"))};
        if(function)
        {
            return function(instance, createInfo, allocator, debugMessanger);
        }
        else
        {
            return VK_ERROR_EXTENSION_NOT_PRESENT;
        }
    }
    
    void destroy_debug_utils_messenger_ext(VkInstance instance, 
                                           VkDebugUtilsMessengerEXT debugMessanger,
                                           const VkAllocationCallbacks* allocator)
    {
        auto function{reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"))};
        if(function)
        {
            function(instance, debugMessanger, allocator);
        }
    }
}
    