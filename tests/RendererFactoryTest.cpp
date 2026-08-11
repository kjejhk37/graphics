#include <gtest/gtest.h>

#include "graphics/renderer/directx11/DirectX11Renderer.h"
#include "graphics/renderer/directx12/DirectX12Renderer.h"
#include "graphics/renderer/directx9/DirectX9Renderer.h"
#include "graphics/renderer/opengl/OpenGLRenderer.h"
#include "graphics/renderer/RendererFactory.h"

TEST(RendererFactoryTest, CreatesDirectX9RendererForDirectX9Backend)
{
    auto renderer = RendererFactory::Create(RendererBackend::DirectX9);
    ASSERT_NE(renderer, nullptr);
    EXPECT_NE(dynamic_cast<DirectX9Renderer*>(renderer.get()), nullptr);
}

TEST(RendererFactoryTest, CreatesDirectX11RendererForDirectX11Backend)
{
    auto renderer = RendererFactory::Create(RendererBackend::DirectX11);
    ASSERT_NE(renderer, nullptr);
    EXPECT_NE(dynamic_cast<DirectX11Renderer*>(renderer.get()), nullptr);
}

TEST(RendererFactoryTest, CreatesDirectX12RendererForDirectX12Backend)
{
    auto renderer = RendererFactory::Create(RendererBackend::DirectX12);
    ASSERT_NE(renderer, nullptr);
    EXPECT_NE(dynamic_cast<DirectX12Renderer*>(renderer.get()), nullptr);
}

TEST(RendererFactoryTest, CreatesOpenGLRendererForOpenGLBackend)
{
    auto renderer = RendererFactory::Create(RendererBackend::OpenGL);
    ASSERT_NE(renderer, nullptr);
    EXPECT_NE(dynamic_cast<OpenGLRenderer*>(renderer.get()), nullptr);
}
