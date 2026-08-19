#pragma once

#include "graphics/renderer/IRenderer.h"

class IUiElementRegistry;

// Author: Claude
// Description: OpenGL 렌더러 스텁. 현재 프로젝트 목표는 DirectX 마이그레이션이며, OpenGL은 향후 확장 대비용 선택지다.
// Input: (해당 없음)
// Output: (해당 없음)
// Notes: 전부 미구현 상태를 나타내는 값만 반환/무동작한다 — OpenGL 실사용은 이번 범위에 포함되지 않는다.
//        SetUiElementRegistry도 no-op이다 — IRenderer 계약(컴파일 통과)만 만족시키며, 실제 위젯
//        렌더링 배선은 OpenGL 백엔드가 실사용되는 시점으로 미룬다(WOT은 현재 OpenGL을 쓰지 않는다).
// Date: 2026-07-19
class OpenGLRenderer final : public IRenderer
{
public:
    bool Initialize(HWND windowHandle, int width, int height) override;
    void RenderFrame(const InstanceSnapshot& snapshot) override;
    void OnResize(int width, int height) override;
    void Shutdown() override;
    bool HandleUiMessage(HWND windowHandle, UINT message, WPARAM wParam, LPARAM lParam) override;
    void SetUiElementRegistry(IUiElementRegistry& registry) override;
    void* LoadTexture(const std::string& filePath) override;
    void UnloadTexture(void* textureHandle) override;
};
