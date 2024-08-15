#include <cstdlib>
#include <iostream>
#include <print>

#include <vulkan/vulkan.hpp>

#include "triangle.hpp"

int main(int argc, char** argv)
{
    app::Triangle program{};
    try
    {
        program.run();
    }
    catch(const std::exception& e)
    {
        std::println(std::cerr, "Error: {}.", e.what());
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}