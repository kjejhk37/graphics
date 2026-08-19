#pragma once

class IUiBase;

// Author: Claude
// Description: 위젯 종류를 모르는 순수 레지스트리 계약. IUiBase 포인터만 다루며 소유권은 갖지 않는다
//              (등록/해제된 위젯의 생성·소멸은 호출자 책임).
// Input: (해당 없음 - 인터페이스)
// Output: (해당 없음 - 인터페이스)
// Notes: Add/Remove는 동일 포인터의 중복 등록 방지나 순서 보장을 요구하지 않는다 - 그 정책은
//        구현체(UiElementRegistry)가 결정한다. RenderAll()은 등록된 모든 위젯의 Render()를 순회
//        호출하며, IUiBase::Render()와 동일하게 활성 ImGui 프레임 안에서 호출되어야 한다.
// Date: 2026-08-19
class IUiElementRegistry
{
public:
    virtual ~IUiElementRegistry() = default;

    virtual void Add(IUiBase* widget) = 0;
    virtual void Remove(IUiBase* widget) = 0;
    virtual void RenderAll() = 0;
};
