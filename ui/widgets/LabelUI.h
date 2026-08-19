#pragma once

#include <string>

#include "graphics/ui/widgets/IUiBase.h"

// Author: Claude
// Description: ImGui::TextUnformatted wrapper.
// Input: 생성자 - text(표시할 문자열)
// Output: (해당 없음 - IUiBase 구현)
// Notes: ImGui::Text(포맷 문자열)가 아닌 TextUnformatted를 쓴다 - text를 포맷 문자열로 취급하면
//        '%'가 포함된 텍스트에서 정의되지 않은 동작이 발생하기 때문이다. 동적으로 바뀌는 텍스트가
//        필요하면 호출자가 SetText()로 갱신한다.
// Date: 2026-08-19
class LabelUI final : public IUiBase
{
public:
    explicit LabelUI(std::string text);

    void SetText(std::string text);
    void Render() override;

private:
    std::string m_text;
};
