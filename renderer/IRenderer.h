#pragma once

#include <windows.h>

#include "graphics/renderer/InstanceSnapshot.h"

// Author: Claude
// Description: DirectX/OpenGL 초기화 절차를 감싸는 렌더러 인터페이스. 각 구현체가 외부 그래픽 API를 캡슐화한다.
// Input: (해당 없음 — 인터페이스)
// Output: (해당 없음 — 인터페이스)
// Notes: HWND는 서드파티 라이브러리가 아닌 Windows 플랫폼 타입이라 외부 라이브러리 wrapper 규칙 대상이 아니다.
//        DirectX 전용 타입(ID3D11Device 등)은 이 인터페이스에 절대 노출하지 않는다 — 그게 이 인터페이스가 존재하는 이유다.
//        OnResize는 창 크기 변경(리사이즈) 시 스왑체인 버퍼를 재생성하기 위해 호출된다.
//        HandleUiMessage는 Win32Window::SetMessageHook을 통해 전달되는 원시 메시지를 각 렌더러가 소유한
//        UI 프레임워크(ImGui 등)로 위임하기 위한 것이다 — Win32Window는 IRenderer/UI 프레임워크를 전혀 모른다(SRP).
//        RenderFrame은 Engine(producer) 스레드가 계산한 최신 InstanceSnapshot을 값으로 전달받는다 —
//        렌더러는 IFrameDataPublisher/스레딩 정책을 전혀 모른다(호출자인 main.cpp가
//        publisher.AcquireReadSnapshot()으로 얻은 순수 데이터만 넘긴다, SRP). InstanceSnapshot은
//        DirectX 타입이 아닌 순수 데이터 구조체라 위 "DirectX 전용 타입 노출 금지" 원칙 위반이 아니다.
// Date: 2026-07-19
class IRenderer
{
public:
    virtual ~IRenderer() = default;

    virtual bool Initialize(HWND windowHandle, int width, int height) = 0;
    virtual void RenderFrame(const InstanceSnapshot& snapshot) = 0;
    virtual void OnResize(int width, int height) = 0;
    virtual void Shutdown() = 0;
    virtual bool HandleUiMessage(HWND windowHandle, UINT message, WPARAM wParam, LPARAM lParam) = 0;
};
