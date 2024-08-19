#include <cstddef>
#include <cstdint>

#include "utils.hpp"

namespace app
{
    VkVertexInputBindingDescription Vertex::binding_description()
    {
        VkVertexInputBindingDescription bindingDesrciption{};
        bindingDesrciption.binding = 0;
        bindingDesrciption.stride = sizeof(Vertex);
        bindingDesrciption.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDesrciption;
    }

    std::array<VkVertexInputAttributeDescription, 3> Vertex::attribute_description()
    {
        std::array<VkVertexInputAttributeDescription, 3> attributeDescription{};
        attributeDescription[0].binding = 0;
        attributeDescription[0].location = 0;
        attributeDescription[0].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescription[0].offset = offsetof(Vertex, position);
        
        attributeDescription[1].binding = 0;
        attributeDescription[1].location = 1;
        attributeDescription[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescription[1].offset = offsetof(Vertex, color);

        attributeDescription[2].binding = 0;
        attributeDescription[2].location = 2;
        attributeDescription[2].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescription[2].offset = offsetof(Vertex, textureCoordinate);

        return attributeDescription;
    }

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
    