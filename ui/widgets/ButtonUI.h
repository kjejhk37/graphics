#pragma once

#include <functional>
#include <string>

#include "graphics/ui/widgets/IUiBase.h"

// Author: Claude
// Description: ImGui::Button wrapper. 라벨과 클릭 콜백만 아는 최소 위젯.
// Input: 생성자 - label(버튼 표시 텍스트이자 ImGui ID로 쓰인다), onClick(클릭 시 호출할 콜백, 기본 없음)
// Output: (해당 없음 - IUiBase 구현)
// Notes: onClick이 비어있으면(std::function이 비어있으면) 클릭돼도 아무 일도 하지 않는다 - 호출부가
//        콜백을 아직 정하지 않은 채 위젯만 먼저 등록하는 시나리오를 허용하기 위함. ImGui 관례상
//        label이 곧 위젯 ID이므로, 같은 화면에 label이 겹치는 ButtonUI 두 개를 두면 ID가 충돌한다
//        (호출자 책임 - ImGui의 표준 동작과 동일).
// Date: 2026-08-19
class ButtonUI final : public IUiBase
{
public:
    explicit ButtonUI(std::string label, std::function<void()> onClick = nullptr);

    void Render() override;

private:
    std::string m_label;
    std::function<void()> m_onClick;
};
