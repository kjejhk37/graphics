#pragma once

#include <vector>

#include "graphics/ui/widgets/IUiElementRegistry.h"

// Author: Claude
// Description: IUiElementRegistry의 기본 구현. std::vector<IUiBase*>로 등록된 위젯을 순서대로 보관한다.
// Input: (해당 없음)
// Output: (해당 없음)
// Notes: non-owning - 저장된 포인터를 delete하지 않는다(수명 관리는 호출자 책임, IUiElementRegistry.h
//        참고). projects(WOT)가 등록할 구체 위젯 타입을 전혀 몰라도 되므로(IUiBase*만 다룸) graphics
//        쪽에 이 구현체를 두어도 역방향 의존이 생기지 않는다(Brainstorming Q1).
// Date: 2026-08-19
class UiElementRegistry final : public IUiElementRegistry
{
public:
    void Add(IUiBase* widget) override;
    void Remove(IUiBase* widget) override;
    void RenderAll() override;

private:
    std::vector<IUiBase*> m_widgets;
};
