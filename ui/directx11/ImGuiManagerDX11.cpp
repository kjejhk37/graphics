#include "graphics/ui/directx11/ImGuiManagerDX11.h"

#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>

// imgui_impl_win32.h는 이 선언을 #if 0 블록 뒤에 감춰둔다 — 전역 스코프에 직접 선언(ImGuiManagerDX9.cpp 참고).
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

ImGuiManagerDX11::ImGuiManagerDX11(HWND windowHandle, ID3D11Device* device, ID3D11DeviceContext* context)
{
    ImGui::CreateContext();
    ImGui_ImplWin32_Init(windowHandle);
    ImGui_ImplDX11_Init(device, context);
}

void ImGuiManagerDX11::NewFrame()
{
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

void ImGuiManagerDX11::Render()
{
    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

void ImGuiManagerDX11::Shutdown()
{
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

bool ImGuiManagerDX11::HandleWin32Message(HWND windowHandle, UINT message, WPARAM wParam, LPARAM lParam)
{
    return ImGui_ImplWin32_WndProcHandler(windowHandle, message, wParam, lParam) != 0;
}
