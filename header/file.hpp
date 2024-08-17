#ifndef FILE_HPP
#define FILE_HPP

#include <vector>
#include <filesystem>
#include <stdexcept>
#include <fstream>

namespace file
{
    std::vector<char> read_file(const std::filesystem::path& filename)
    {
        std::ifstream in{filename, std::ios::ate | std::ios::binary};

        if(!in.is_open())
        {
            throw std::runtime_error{"Error: failed to open file."};
        }

        std::size_t fileSize{in.tellg()};
        std::vector<char> buffer(fileSize);

        in.seekg(0);
        in.read(std::data(buffer), fileSize);

        in.close();
        return buffer;
    }
}

#endif