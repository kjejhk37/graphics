#include <gtest/gtest.h>

#include <wrl/client.h>

#include <d3d11.h>

#include <string>

#include "platform/Win32Window.h"
#include "graphics/ui/directx11/ImGuiManagerDX11.h"
#include "graphics/ui/widgets/UiElementRegistry.h"
#include "graphics/ui/widgets/ButtonUI.h"
#include "graphics/ui/widgets/ProgressUI.h"
#include "graphics/ui/widgets/StringFieldUI.h"
#include "graphics/ui/widgets/LabelUI.h"
#include "graphics/ui/widgets/ImageUI.h"

namespace
{
    bool CreateWarpDeviceAndContext(Microsoft::WRL::ComPtr<ID3D11Device>& device,
                                     Microsoft::WRL::ComPtr<ID3D11DeviceContext>& context)
    {
        const HRESULT hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, 0, nullptr, 0,
                                               D3D11_SDK_VERSION, device.GetAddressOf(), nullptr, context.GetAddressOf());
        return SUCCEEDED(hr);
    }
}

// 실제 위젯 wrapper는 활성 ImGui 프레임(NewFrame()~Render() 사이) 안에서만 호출할 수 있으므로
// (IUiBase.h Notes 참고), ImGuiManagerDX11Test.cpp와 동일하게 WARP 디바이스로 진짜 프레임을 연다.
TEST(UiWidgetsTest, AllWidgetsRenderWithoutCrashInsideActiveFrame)
{
    Win32Window window(640, 480, "UiWidgetsTest", false);
    Microsoft::WRL::ComPtr<ID3D11Device> device;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;
    ASSERT_TRUE(CreateWarpDeviceAndContext(device, context));

    ImGuiManagerDX11 uiManager(window.Handle(), device.Get(), context.Get());

    std::string fieldValue = "hello";
    float progress = 0.5f;
    int buttonClickCount = 0;
    int imageClickCount = 0;

    ButtonUI button("Click Me", [&buttonClickCount]() { ++buttonClickCount; });
    ProgressUI progressBar(&progress, "50%");
    StringFieldUI stringField("Name", &fieldValue);
    LabelUI label("Hello Label");
    ImageUI staticImage(nullptr, 64.0f, 64.0f);
    ImageUI clickableImage(nullptr, 64.0f, 64.0f, [&imageClickCount]() { ++imageClickCount; });

    UiElementRegistry registry;
    registry.Add(&button);
    registry.Add(&progressBar);
    registry.Add(&stringField);
    registry.Add(&label);
    registry.Add(&staticImage);
    registry.Add(&clickableImage);

    uiManager.NewFrame();
    EXPECT_NO_FATAL_FAILURE(registry.RenderAll());
    uiManager.Render();
    uiManager.Shutdown();
}

TEST(UiWidgetsTest, LabelUiSetTextDoesNotCrashOnNextRender)
{
    Win32Window window(640, 480, "UiWidgetsTest", false);
    Microsoft::WRL::ComPtr<ID3D11Device> device;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;
    ASSERT_TRUE(CreateWarpDeviceAndContext(device, context));

    ImGuiManagerDX11 uiManager(window.Handle(), device.Get(), context.Get());

    LabelUI label("Before");
    label.SetText("After");

    uiManager.NewFrame();
    EXPECT_NO_FATAL_FAILURE(label.Render());
    uiManager.Render();
    uiManager.Shutdown();
}
