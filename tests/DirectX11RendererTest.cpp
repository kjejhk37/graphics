#include <gtest/gtest.h>

#include <vector>

#include "graphics/renderer/InstanceSnapshot.h"
#include "platform/math/Matrix4x4.h"
#include "platform/math/Quaternion.h"
#include "platform/math/Vec3.h"
#include "platform/Win32Window.h"
#include "graphics/renderer/directx11/DirectX11Renderer.h"
#include "graphics/ui/widgets/IUiBase.h"
#include "graphics/ui/widgets/IUiElementRegistry.h"

namespace
{
    class SpyUiElementRegistry final : public IUiElementRegistry
    {
    public:
        void Add(IUiBase* /*widget*/) override {}
        void Remove(IUiBase* /*widget*/) override {}
        void RenderAll() override { ++renderAllCallCount; }

        int renderAllCallCount = 0;
    };
}

TEST(DirectX11RendererTest, InitializeSucceedsWithForcedWarp)
{
    Win32Window window(640, 480, "DirectX11RendererTest", false);

    DirectX11Renderer renderer(/*forceWarp=*/true);
    EXPECT_TRUE(renderer.Initialize(window.Handle(), 640, 480));
    renderer.Shutdown();
}

TEST(DirectX11RendererTest, InitializeSucceedsWithDefaultDriverSelection)
{
    Win32Window window(640, 480, "DirectX11RendererTest", false);

    DirectX11Renderer renderer;
    EXPECT_TRUE(renderer.Initialize(window.Handle(), 640, 480));
    renderer.Shutdown();
}

TEST(DirectX11RendererTest, SurvivesResizeAndRenderFrame)
{
    Win32Window window(640, 480, "DirectX11RendererTest", false);

    DirectX11Renderer renderer(/*forceWarp=*/true);
    ASSERT_TRUE(renderer.Initialize(window.Handle(), 640, 480));

    renderer.OnResize(800, 600);
    renderer.RenderFrame(InstanceSnapshot{});

    renderer.OnResize(0, 0);  // 최소화 시나리오 — 크래시 없이 무시되어야 함
    renderer.RenderFrame(InstanceSnapshot{});

    renderer.Shutdown();
}

TEST(DirectX11RendererTest, SetUiElementRegistryInvokesRenderAllDuringRenderFrame)
{
    Win32Window window(640, 480, "DirectX11RendererTest", false);

    DirectX11Renderer renderer(/*forceWarp=*/true);
    ASSERT_TRUE(renderer.Initialize(window.Handle(), 640, 480));

    SpyUiElementRegistry registry;
    renderer.SetUiElementRegistry(registry);
    renderer.RenderFrame(InstanceSnapshot{});

    EXPECT_EQ(registry.renderAllCallCount, 1);

    renderer.Shutdown();
}

TEST(DirectX11RendererTest, InstancedRenderFrameUploadsExactWorldMatricesToInstanceBuffer)
{
    Win32Window window(640, 480, "DirectX11RendererTest", false);

    DirectX11Renderer renderer(/*forceWarp=*/true);
    ASSERT_TRUE(renderer.Initialize(window.Handle(), 640, 480));

    InstanceSnapshot snapshot;
    for (int i = 0; i < 5; ++i)
    {
        const Vec3 position(static_cast<float>(i) * 2.0f, 0.0f, 0.0f);
        const Quaternion rotation = Quaternion::FromAxisAngle(Vec3(0.0f, 1.0f, 0.0f), static_cast<float>(i) * 15.0f);
        snapshot.worldMatrices.push_back(Matrix4x4::FromTRS(position, rotation, Vec3::One()));
    }

    renderer.RenderFrame(snapshot);

    std::vector<Matrix4x4> readBack;
    ASSERT_TRUE(renderer.DebugReadBackInstanceBuffer(readBack));
    ASSERT_EQ(readBack.size(), snapshot.worldMatrices.size());
    for (std::size_t i = 0; i < snapshot.worldMatrices.size(); ++i)
    {
        for (int row = 0; row < 4; ++row)
        {
            for (int col = 0; col < 4; ++col)
            {
                EXPECT_FLOAT_EQ(readBack[i].m[row][col], snapshot.worldMatrices[i].m[row][col]);
            }
        }
    }

    renderer.Shutdown();
}
