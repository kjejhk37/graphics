#pragma once

#include <windows.h>

#include <wrl/client.h>

#include <d3d12.h>
#include <dxgi.h>

#include "graphics/ui/IUiManager.h"

// Author: Claude
// Description: IUiManager의 DirectX 12 구현. <imgui.h>/<imgui_impl_dx12.h>/<imgui_impl_win32.h>를 아는 유일한 파일.
// Input: 생성자 - windowHandle(ImGui Win32 백엔드용), device/commandQueue(ImGui DX12 백엔드용),
//        numFramesInFlight(스왑체인 버퍼 수와 일치, DirectX12Renderer::kBackBufferCount 재사용),
//        rtvFormat(스왑체인 렌더 타겟 포맷)
// Output: (해당 없음 - IUiManager 인터페이스 구현)
// Notes: DX12는 폰트 텍스처용으로 SRV(CBV_SRV_UAV) 디스크립터 힙이 별도로 필요하다 — DirectX12Renderer가
//        이미 갖고 있는 RTV 힙과는 완전히 별개이며, 이 클래스가 직접 소유한다.
//        DX12는 draw call 기록에 커맨드 리스트가 필요해 IUiManager::Render()(무인자)로는 표현할 수 없다 —
//        그래서 실제 렌더링은 RenderWithCommandList(인터페이스 밖 전용 메서드)로 수행하고,
//        Render()는 이 클래스를 IUiManager로만 다루는 경로가 실수로 호출해도 안전하도록 no-op으로 둔다.
//        이 클래스를 소유하는 DirectX12Renderer는 IUiManager*가 아니라 ImGuiManagerDX12 구체 타입을
//        직접 들고 있어 항상 RenderWithCommandList를 호출한다.
// Date: 2026-07-19
class ImGuiManagerDX12 final : public IUiManager
{
public:
    ImGuiManagerDX12(HWND windowHandle, ID3D12Device* device, ID3D12CommandQueue* commandQueue,
                      int numFramesInFlight, DXGI_FORMAT rtvFormat);

    void NewFrame() override;
    void Render() override;
    void Shutdown() override;
    bool HandleWin32Message(HWND windowHandle, UINT message, WPARAM wParam, LPARAM lParam) override;

    void RenderWithCommandList(ID3D12GraphicsCommandList* commandList);

private:
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_srvDescriptorHeap;
};
