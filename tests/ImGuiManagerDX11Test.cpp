#include <gtest/gtest.h>

#include <wrl/client.h>

#include <d3d11.h>

#include "platform/Win32Window.h"
#include "graphics/ui/directx11/ImGuiManagerDX11.h"

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

TEST(ImGuiManagerDX11Test, NewFrameRenderShutdownDoNotCrash)
{
    Win32Window window(640, 480, "ImGuiManagerDX11Test", false);
    Microsoft::WRL::ComPtr<ID3D11Device> device;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;
    ASSERT_TRUE(CreateWarpDeviceAndContext(device, context));

    ImGuiManagerDX11 uiManager(window.Handle(), device.Get(), context.Get());
    uiManager.NewFrame();
    uiManager.Render();
    uiManager.Shutdown();
}

TEST(ImGuiManagerDX11Test, HandleWin32MessageDoesNotCrash)
{
    Win32Window window(640, 480, "ImGuiManagerDX11Test", false);
    Microsoft::WRL::ComPtr<ID3D11Device> device;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;
    ASSERT_TRUE(CreateWarpDeviceAndContext(device, context));

    ImGuiManagerDX11 uiManager(window.Handle(), device.Get(), context.Get());
    EXPECT_NO_FATAL_FAILURE(uiManager.HandleWin32Message(window.Handle(), WM_MOUSEMOVE, 0, 0));
    uiManager.Shutdown();
}
