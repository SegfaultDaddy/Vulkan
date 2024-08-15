#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "triangle.hpp"

namespace app
{
    Triangle::Triangle(const std::uint32_t width, const std::uint32_t height, const std::string_view name)
    {
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        window = glfwCreateWindow(width, height, std::data(name), nullptr, nullptr);

    }
    
    Triangle::~Triangle()
    {
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
} 