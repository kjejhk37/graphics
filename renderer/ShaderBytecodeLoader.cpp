#include "graphics/renderer/ShaderBytecodeLoader.h"

#include <fstream>
#include <stdexcept>

std::vector<uint8_t> ShaderBytecodeLoader::Load(const std::string& csoPath)
{
    std::ifstream file(csoPath, std::ios::binary | std::ios::ate);
    if (!file)
    {
        throw std::runtime_error("Failed to open shader bytecode file: " + csoPath);
    }

    const std::streamsize size = file.tellg();
    if (size <= 0)
    {
        throw std::runtime_error("Shader bytecode file is empty: " + csoPath);
    }

    file.seekg(0, std::ios::beg);
    std::vector<uint8_t> bytes(static_cast<std::size_t>(size));
    if (!file.read(reinterpret_cast<char*>(bytes.data()), size))
    {
        throw std::runtime_error("Failed to read shader bytecode file: " + csoPath);
    }

    return bytes;
}
