#pragma once

#include <string>

#include "graphics/ui/widgets/IUiBase.h"

// Author: Claude
// Description: ImGui::ProgressBar wrapper.
// Input: 생성자 - fraction(진행률 0~1을 가리키는 외부 소유 값의 포인터 - 매 프레임 최신값을 읽기
//        위해 값 복사 대신 포인터로 참조), overlayLabel(진행률 바 위에 겹쳐 그릴 문자열, 기본 없음)
// Output: (해당 없음 - IUiBase 구현)
// Notes: fraction은 위젯이 소유하지 않는다 - 매 프레임 최신 진행률을 반영하려면 호출자가 그 값을
//        직접 갱신해야 한다(예: 로딩 진행률 변수를 가리키게 함). fraction이 nullptr이면 0으로 그린다.
// Date: 2026-08-19
class ProgressUI final : public IUiBase
{
public:
    explicit ProgressUI(const float* fraction, std::string overlayLabel = "");

    void Render() override;

private:
    const float* m_fraction;
    std::string m_overlayLabel;
};
