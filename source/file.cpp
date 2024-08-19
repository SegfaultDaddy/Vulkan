#include <stdexcept>
#include <fstream>

#include "file.hpp"

namespace file
{
    std::vector<char> read_file(const std::filesystem::path& filename)
    {
        std::ifstream in{filename, std::ios::ate | std::ios::binary};

        if(!in.is_open())
        {
            throw std::runtime_error{"Error: failed to open file."};
        }

        std::size_t fileSize{static_cast<std::size_t>(in.tellg())};
        std::vector<char> buffer(fileSize);

        in.seekg(0);
        in.read(std::data(buffer), fileSize);

        in.close();
        return buffer;
    }
}
