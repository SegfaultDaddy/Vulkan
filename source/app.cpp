#include <cstdlib>
#include <print>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

int main(int argc, char** argv)
{
    if(!glfwInit())
    {
        return EXIT_FAILURE;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window{glfwCreateWindow(800, 600, "Vulkan window", nullptr, nullptr)};

    if(window == nullptr)
    {
        return EXIT_FAILURE;
    }

    std::uint32_t extensionCount{0};
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

    std::println("{} extensions supported", extensionCount);

    glm::mat4 matrix{};
    glm::vec4 vector{};

    auto test{matrix * vector};
    while(!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return EXIT_SUCCESS;
}