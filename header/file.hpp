#ifndef FILE_HPP
#define FILE_HPP

#include <vector>
#include <filesystem>

namespace file
{
    std::vector<char> read_file(const std::filesystem::path& filename); 
}

#endif