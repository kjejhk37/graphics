#include <gtest/gtest.h>

#include <wrl/client.h>

#include <d3d9.h>

#include "platform/Win32Window.h"
#include "graphics/ui/directx9/ImGuiManagerDX9.h"

namespace
{
    Microsoft::WRL::ComPtr<IDirect3DDevice9> CreateHalDevice(HWND windowHandle)
    {
        Microsoft::WRL::ComPtr<IDirect3D9> direct3D;
        direct3D.Attach(Direct3DCreate9(D3D_SDK_VERSION));
        if (!direct3D)
        {
            return nullptr;
        }

        D3DPRESENT_PARAMETERS presentParams{};
        presentParams.Windowed = TRUE;
        presentParams.SwapEffect = D3DSWAPEFFECT_DISCARD;
        presentParams.BackBufferFormat = D3DFMT_UNKNOWN;
        presentParams.hDeviceWindow = windowHandle;
        presentParams.BackBufferWidth = 640;
        presentParams.BackBufferHeight = 480;

        Microsoft::WRL::ComPtr<IDirect3DDevice9> device;
        const HRESULT hr = direct3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, windowHandle,
                                                    D3DCREATE_HARDWARE_VERTEXPROCESSING, &presentParams,
                                                    device.GetAddressOf());
        return SUCCEEDED(hr) ? device : nullptr;
    }
}

// Note: DirectX9RendererTest와 동일하게 실제 그래픽 드라이버(D3DDEVTYPE_HAL)가 있는 환경을 가정한다.
TEST(ImGuiManagerDX9Test, NewFrameRenderShutdownDoNotCrash)
{
    Win32Window window(640, 480, "ImGuiManagerDX9Test", false);
    Microsoft::WRL::ComPtr<IDirect3DDevice9> device = CreateHalDevice(window.Handle());
    ASSERT_TRUE(device != nullptr);

    ImGuiManagerDX9 uiManager(window.Handle(), device.Get());
    uiManager.NewFrame();

    device->BeginScene();
    uiManager.Render();
    device->EndScene();

    uiManager.Shutdown();
}

TEST(ImGuiManagerDX9Test, HandleWin32MessageDoesNotCrash)
{
    Win32Window window(640, 480, "ImGuiManagerDX9Test", false);
    Microsoft::WRL::ComPtr<IDirect3DDevice9> device = CreateHalDevice(window.Handle());
    ASSERT_TRUE(device != nullptr);

    ImGuiManagerDX9 uiManager(window.Handle(), device.Get());
    EXPECT_NO_FATAL_FAILURE(uiManager.HandleWin32Message(window.Handle(), WM_MOUSEMOVE, 0, 0));
    uiManager.Shutdown();
}
