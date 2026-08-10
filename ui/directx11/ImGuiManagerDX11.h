#pragma once

#include <windows.h>

#include <d3d11.h>

#include "graphics/ui/IUiManager.h"

// Author: Claude
// Description: IUiManager의 DirectX 11 구현. <imgui.h>/<imgui_impl_dx11.h>/<imgui_impl_win32.h>를 아는 유일한 파일.
// Input: 생성자 - windowHandle(ImGui Win32 백엔드용 HWND), device/context(ImGui DX11 백엔드용)
// Output: (해당 없음 - IUiManager 인터페이스 구현)
// Notes: ImGui::CreateContext는 생성자에서, ImGui::DestroyContext는 Shutdown에서 호출한다 — 이 앱은
//        한 번에 하나의 렌더러 백엔드만 활성화하므로 여러 ImGuiManagerDXn 인스턴스가 동시에
//        컨텍스트를 소유할 일은 없다(ImGuiManagerDX9.h와 동일한 전제).
// Date: 2026-07-19
class ImGuiManagerDX11 final : public IUiManager
{
public:
    ImGuiManagerDX11(HWND windowHandle, ID3D11Device* device, ID3D11DeviceContext* context);

    void NewFrame() override;
    void Render() override;
    void Shutdown() override;
    bool HandleWin32Message(HWND windowHandle, UINT message, WPARAM wParam, LPARAM lParam) override;
};
