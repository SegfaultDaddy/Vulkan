#include <cstdlib>
#include <iostream>
#include <print>

#include "triangle.hpp"

int main(int argc, char** argv)
{
    try
    {
        app::Triangle program{800, 600};
        program.run();
    }
    catch(const std::exception& e)
    {
        std::println(std::cerr, "{}", e.what());
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}