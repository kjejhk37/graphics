#include <gtest/gtest.h>

#include <wrl/client.h>

#include <d3d12.h>
#include <dxgi1_4.h>

#include "platform/Win32Window.h"
#include "graphics/ui/directx12/ImGuiManagerDX12.h"

namespace
{
    bool CreateWarpDeviceAndQueue(Microsoft::WRL::ComPtr<ID3D12Device>& device,
                                  Microsoft::WRL::ComPtr<ID3D12CommandQueue>& commandQueue)
    {
        Microsoft::WRL::ComPtr<IDXGIFactory4> factory;
        if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(factory.GetAddressOf()))))
        {
            return false;
        }

        Microsoft::WRL::ComPtr<IDXGIAdapter> warpAdapter;
        if (FAILED(factory->EnumWarpAdapter(IID_PPV_ARGS(warpAdapter.GetAddressOf()))))
        {
            return false;
        }

        if (FAILED(D3D12CreateDevice(warpAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(device.GetAddressOf()))))
        {
            return false;
        }

        D3D12_COMMAND_QUEUE_DESC queueDesc{};
        queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        return SUCCEEDED(device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(commandQueue.GetAddressOf())));
    }
}

TEST(ImGuiManagerDX12Test, NewFrameRenderShutdownDoNotCrash)
{
    Win32Window window(640, 480, "ImGuiManagerDX12Test", false);
    Microsoft::WRL::ComPtr<ID3D12Device> device;
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue;
    ASSERT_TRUE(CreateWarpDeviceAndQueue(device, commandQueue));

    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator;
    ASSERT_TRUE(SUCCEEDED(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
                                                          IID_PPV_ARGS(commandAllocator.GetAddressOf()))));
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList;
    ASSERT_TRUE(SUCCEEDED(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator.Get(), nullptr,
                                                     IID_PPV_ARGS(commandList.GetAddressOf()))));

    ImGuiManagerDX12 uiManager(window.Handle(), device.Get(), commandQueue.Get(), /*numFramesInFlight=*/2,
                                DXGI_FORMAT_R8G8B8A8_UNORM);
    uiManager.NewFrame();
    uiManager.RenderWithCommandList(commandList.Get());
    commandList->Close();

    uiManager.Shutdown();
}

TEST(ImGuiManagerDX12Test, HandleWin32MessageDoesNotCrash)
{
    Win32Window window(640, 480, "ImGuiManagerDX12Test", false);
    Microsoft::WRL::ComPtr<ID3D12Device> device;
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue;
    ASSERT_TRUE(CreateWarpDeviceAndQueue(device, commandQueue));

    ImGuiManagerDX12 uiManager(window.Handle(), device.Get(), commandQueue.Get(), /*numFramesInFlight=*/2,
                                DXGI_FORMAT_R8G8B8A8_UNORM);
    EXPECT_NO_FATAL_FAILURE(uiManager.HandleWin32Message(window.Handle(), WM_MOUSEMOVE, 0, 0));
    uiManager.Shutdown();
}
