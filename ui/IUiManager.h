#pragma once

#include <windows.h>

// Author: Claude
// Description: ImGui 등 즉시 모드 UI 라이브러리의 프레임 생명주기를 감싸는 인터페이스.
//              각 구현체가 특정 UI 라이브러리/렌더러 백엔드를 완전히 캡슐화한다.
// Input: (해당 없음 — 인터페이스)
// Output: (해당 없음 — 인터페이스)
// Notes: Initialize는 이 인터페이스에 없다 — DX9/11/12 백엔드마다 필요한 파라미터(디바이스/컨텍스트/
//        디스크립터 힙 등)가 완전히 달라 공통 시그니처로 표현할 수 없기 때문이다. 각 구현체가 자신에게
//        맞는 생성자를 갖고, 이를 소유하는 렌더러가 구체 타입으로 직접 생성한다.
//        HWND는 서드파티 라이브러리가 아닌 Windows 플랫폼 타입이라 외부 라이브러리 wrapper 규칙 대상이 아니다
//        (IRenderer.h와 동일한 원칙).
// Date: 2026-07-19
class IUiManager
{
public:
    virtual ~IUiManager() = default;

    virtual void NewFrame() = 0;
    virtual void Render() = 0;
    virtual void Shutdown() = 0;
    virtual bool HandleWin32Message(HWND windowHandle, UINT message, WPARAM wParam, LPARAM lParam) = 0;
};
