#include "graphics/ui/directx9/ImGuiManagerDX9.h"

#include <imgui.h>
#include <imgui_impl_dx9.h>
#include <imgui_impl_win32.h>

// imgui_impl_win32.h는 이 선언을 #if 0 블록 뒤에 감춰둔다 — ImGui 공식 예제와 동일하게
// 애플리케이션(백엔드 사용자) 쪽에서 전역 스코프에 직접 선언해서 쓴다.
// 익명 네임스페이스에 넣으면 이름이 맹글링되어 imgui_impl_win32.cpp가 내보내는 실제 심볼과
// 링크되지 않으므로 반드시 전역 스코프에 선언해야 한다.
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

ImGuiManagerDX9::ImGuiManagerDX9(HWND windowHandle, IDirect3DDevice9* device)
{
    ImGui::CreateContext();
    ImGui_ImplWin32_Init(windowHandle);
    ImGui_ImplDX9_Init(device);
}

void ImGuiManagerDX9::NewFrame()
{
    ImGui_ImplDX9_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

void ImGuiManagerDX9::Render()
{
    ImGui::Render();
    ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
}

void ImGuiManagerDX9::Shutdown()
{
    ImGui_ImplDX9_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

bool ImGuiManagerDX9::HandleWin32Message(HWND windowHandle, UINT message, WPARAM wParam, LPARAM lParam)
{
    return ImGui_ImplWin32_WndProcHandler(windowHandle, message, wParam, lParam) != 0;
}
