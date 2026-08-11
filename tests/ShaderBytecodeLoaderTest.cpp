#include <gtest/gtest.h>

#include <stdexcept>
#include <string>

#include "graphics/renderer/ShaderBytecodeLoader.h"

TEST(ShaderBytecodeLoaderTest, LoadReturnsNonEmptyBytesForCompiledShader)
{
    const std::string csoPath = std::string(GRAPHICS_SHADER_OUTPUT_DIR) + "directx11/Baseline.vs.cso";

    const std::vector<uint8_t> bytes = ShaderBytecodeLoader::Load(csoPath);

    EXPECT_FALSE(bytes.empty());
}

TEST(ShaderBytecodeLoaderTest, LoadThrowsWhenFileDoesNotExist)
{
    EXPECT_THROW(ShaderBytecodeLoader::Load("nonexistent_shader_file.cso"), std::runtime_error);
}
