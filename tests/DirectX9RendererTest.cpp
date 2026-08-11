#include <gtest/gtest.h>

#include <vector>

#include "graphics/renderer/InstanceSnapshot.h"
#include "platform/math/Matrix4x4.h"
#include "platform/math/Quaternion.h"
#include "platform/math/Vec3.h"
#include "platform/Win32Window.h"
#include "graphics/renderer/directx9/DirectX9Renderer.h"

namespace
{
    InstanceSnapshot MakeSampleSnapshot(int count)
    {
        InstanceSnapshot snapshot;
        for (int i = 0; i < count; ++i)
        {
            const Vec3 position(static_cast<float>(i) * 2.0f, 0.0f, 0.0f);
            const Quaternion rotation =
                Quaternion::FromAxisAngle(Vec3(0.0f, 1.0f, 0.0f), static_cast<float>(i) * 15.0f);
            snapshot.worldMatrices.push_back(Matrix4x4::FromTRS(position, rotation, Vec3::One()));
        }
        return snapshot;
    }
}

// Note: 이 테스트는 실제 그래픽 드라이버(D3DDEVTYPE_HAL)가 있는 환경에서 실행된다고 가정한다.
// DX9의 소프트웨어 대체 경로(REF/NULLREF)는 레거시 SDK 컴포넌트가 있어야 안정적으로 동작해 자동 테스트에 쓰지 않는다.
TEST(DirectX9RendererTest, InitializeSucceedsOnRealHardware)
{
    Win32Window window(640, 480, "DirectX9RendererTest", false);

    DirectX9Renderer renderer;
    EXPECT_TRUE(renderer.Initialize(window.Handle(), 640, 480));
    renderer.Shutdown();
}

TEST(DirectX9RendererTest, SurvivesResizeAndRenderFrame)
{
    Win32Window window(640, 480, "DirectX9RendererTest", false);

    DirectX9Renderer renderer;
    ASSERT_TRUE(renderer.Initialize(window.Handle(), 640, 480));

    renderer.OnResize(800, 600);
    renderer.RenderFrame(InstanceSnapshot{});

    renderer.OnResize(0, 0);  // 최소화 시나리오 — 크래시 없이 무시되어야 함
    renderer.RenderFrame(InstanceSnapshot{});

    renderer.Shutdown();
}

TEST(DirectX9RendererTest, InstancedRenderFrameSucceedsAndUploadsMatricesWhenHardwareInstancingAvailable)
{
    Win32Window window(640, 480, "DirectX9RendererTest", false);

    DirectX9Renderer renderer;
    ASSERT_TRUE(renderer.Initialize(window.Handle(), 640, 480));

    const InstanceSnapshot snapshot = MakeSampleSnapshot(5);
    renderer.RenderFrame(snapshot);

    // 하드웨어가 SetStreamSourceFreq 인스턴싱을 지원하면 인스턴스 버퍼가 만들어지고 그 내용이
    // 업로드한 행렬과 정확히 일치해야 한다. 지원하지 않는 드라이버라면 크래시 없이 폴백으로
    // 전환됐을 것이므로(아래 forced-fallback 테스트가 그 경로를 명시적으로 검증), 인스턴스
    // 버퍼가 아예 없을 수도 있다(false) - 이 경우도 유효한 결과다.
    std::vector<Matrix4x4> readBack;
    if (renderer.DebugReadBackInstanceBuffer(readBack))
    {
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
    }

    renderer.Shutdown();
}

TEST(DirectX9RendererTest, InstancedRenderFrameSucceedsWithForcedFallback)
{
    Win32Window window(640, 480, "DirectX9RendererTest", false);

    DirectX9Renderer renderer(/*forceInstancingFallback=*/true);
    ASSERT_TRUE(renderer.Initialize(window.Handle(), 640, 480));

    const InstanceSnapshot snapshot = MakeSampleSnapshot(5);
    renderer.RenderFrame(snapshot);  // 크래시 없이 폴백(개별 DrawIndexedPrimitive 반복)으로 동작해야 함

    // 강제 폴백 경로는 인스턴스 버퍼를 아예 만들지 않는다.
    std::vector<Matrix4x4> readBack;
    EXPECT_FALSE(renderer.DebugReadBackInstanceBuffer(readBack));

    renderer.Shutdown();
}
