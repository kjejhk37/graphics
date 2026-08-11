#include <gtest/gtest.h>

#include "graphics/renderer/RendererBackendParser.h"

TEST(RendererBackendParserTest, ParsesKnownValues)
{
    EXPECT_EQ(RendererBackendParser::TryParse("directx9"), RendererBackend::DirectX9);
    EXPECT_EQ(RendererBackendParser::TryParse("directx11"), RendererBackend::DirectX11);
    EXPECT_EQ(RendererBackendParser::TryParse("directx12"), RendererBackend::DirectX12);
    EXPECT_EQ(RendererBackendParser::TryParse("opengl"), RendererBackend::OpenGL);
}

TEST(RendererBackendParserTest, ReturnsNulloptForUnknownValue)
{
    EXPECT_FALSE(RendererBackendParser::TryParse("foo").has_value());
    EXPECT_FALSE(RendererBackendParser::TryParse("directx").has_value());
}
