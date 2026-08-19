#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "graphics/ui/widgets/IUiBase.h"

// Author: Claude
// Description: ImGui::InputText wrapper.
// Input: 생성자 - label(필드 라벨이자 ImGui ID), buffer(입력 결과를 담을 외부 소유 문자열 포인터 -
//        InputText가 직접 편집할 수 있도록 값이 아닌 포인터로 받는다), bufferCapacity(내부 편집
//        버퍼 크기, 기본 256)
// Output: (해당 없음 - IUiBase 구현)
// Notes: ImGui::InputText는 고정 크기 char* 버퍼를 요구한다 - std::string을 직접 편집 대상으로 넘길
//        수 없어, 내부에 고정 크기 char 버퍼를 두고 Render() 호출마다 *buffer <-> 내부 버퍼를
//        동기화한다. bufferCapacity를 넘는 입력은 잘린다.
// Date: 2026-08-19
class StringFieldUI final : public IUiBase
{
public:
    StringFieldUI(std::string label, std::string* buffer, std::size_t bufferCapacity = 256);

    void Render() override;

private:
    std::string m_label;
    std::string* m_buffer;
    std::vector<char> m_editBuffer;
};
