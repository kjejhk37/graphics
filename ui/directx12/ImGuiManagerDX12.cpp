#include "graphics/ui/directx12/ImGuiManagerDX12.h"

#include <cassert>

#include <imgui.h>
#include <imgui_impl_dx12.h>
#include <imgui_impl_win32.h>

// imgui_impl_win32.h는 이 선언을 #if 0 블록 뒤에 감춰둔다 — 전역 스코프에 직접 선언(ImGuiManagerDX9.cpp 참고).
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace
{
    // 이번 사이클은 ImGui 폰트 아틀라스 텍스처 하나만 쓰므로(실제 UI 위젯을 만들지 않음),
    // 힙의 유일한 슬롯을 그대로 내주는 것으로 충분하다 — 여러 텍스처를 동적으로 할당/반환해야 하는
    // 실제 UI 제작 사이클에서는 이 두 함수를 진짜 풀 할당자로 교체해야 한다.
    void AllocateSrvDescriptor(ImGui_ImplDX12_InitInfo* info, D3D12_CPU_DESCRIPTOR_HANDLE* outCpuHandle,
                                D3D12_GPU_DESCRIPTOR_HANDLE* outGpuHandle)
    {
        *outCpuHandle = info->SrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
        *outGpuHandle = info->SrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
    }

    void FreeSrvDescriptor(ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE)
    {
        // 힙에 디스크립터가 1개뿐이라 반환할 풀이 없다 — no-op.
    }
}

ImGuiManagerDX12::ImGuiManagerDX12(HWND windowHandle, ID3D12Device* device, ID3D12CommandQueue* commandQueue,
                                     int numFramesInFlight, DXGI_FORMAT rtvFormat)
{
    D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc{};
    srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    srvHeapDesc.NumDescriptors = 1;
    srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(m_srvDescriptorHeap.GetAddressOf()));

    ImGui::CreateContext();
    ImGui_ImplWin32_Init(windowHandle);

    ImGui_ImplDX12_InitInfo initInfo{};
    initInfo.Device = device;
    initInfo.CommandQueue = commandQueue;
    initInfo.NumFramesInFlight = numFramesInFlight;
    initInfo.RTVFormat = rtvFormat;
    initInfo.DSVFormat = DXGI_FORMAT_UNKNOWN;
    initInfo.SrvDescriptorHeap = m_srvDescriptorHeap.Get();
    initInfo.SrvDescriptorAllocFn = AllocateSrvDescriptor;
    initInfo.SrvDescriptorFreeFn = FreeSrvDescriptor;
    ImGui_ImplDX12_Init(&initInfo);
}

void ImGuiManagerDX12::NewFrame()
{
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

void ImGuiManagerDX12::Render()
{
    // 커맨드 리스트가 없어 실제로 그릴 수 없다 — 정상 경로에서는 호출되지 않는다(RenderWithCommandList 참고).
    // 디버그 빌드에서는 오용(IUiManager*로 다뤄 이 경로를 호출)을 조용히 넘기지 않고 즉시 실패시킨다.
    assert(false && "ImGuiManagerDX12::Render()는 지원되지 않는다 - RenderWithCommandList를 사용할 것");
}

void ImGuiManagerDX12::Shutdown()
{
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

bool ImGuiManagerDX12::HandleWin32Message(HWND windowHandle, UINT message, WPARAM wParam, LPARAM lParam)
{
    return ImGui_ImplWin32_WndProcHandler(windowHandle, message, wParam, lParam) != 0;
}

void ImGuiManagerDX12::RenderWithCommandList(ID3D12GraphicsCommandList* commandList)
{
    ImGui::Render();

    ID3D12DescriptorHeap* heaps[] = {m_srvDescriptorHeap.Get()};
    commandList->SetDescriptorHeaps(1, heaps);
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);
}
