#pragma once

#include <windows.h>

#include <d3d9.h>

#include "graphics/ui/IUiManager.h"

// Author: Claude
// Description: IUiManager의 DirectX 9 구현. <imgui.h>/<imgui_impl_dx9.h>/<imgui_impl_win32.h>를 아는 유일한 파일.
// Input: 생성자 - windowHandle(ImGui Win32 백엔드용 HWND), device(ImGui DX9 백엔드용 IDirect3DDevice9*)
// Output: (해당 없음 - IUiManager 인터페이스 구현)
// Notes: ImGui::CreateContext는 생성자에서, ImGui::DestroyContext는 Shutdown에서 호출한다 — 이 앱은
//        한 번에 하나의 렌더러 백엔드만 활성화하므로(런타임 전환 시 이전 렌더러가 완전히 Shutdown된 뒤
//        다음 렌더러가 생성됨) 여러 ImGuiManagerDXn 인스턴스가 동시에 컨텍스트를 소유할 일은 없다.
// Date: 2026-07-19
class ImGuiManagerDX9 final : public IUiManager
{
public:
    ImGuiManagerDX9(HWND windowHandle, IDirect3DDevice9* device);

    void NewFrame() override;
    void Render() override;
    void Shutdown() override;
    bool HandleWin32Message(HWND windowHandle, UINT message, WPARAM wParam, LPARAM lParam) override;
};
