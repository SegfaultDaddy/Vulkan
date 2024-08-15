#ifndef TRIANGLE_APP_HPP
#define TRIANGLE_APP_HPP

#include <cstdint>
#include <string_view>

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
        GLFWwindow* window;
    };
}

#endif