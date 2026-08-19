#pragma once

#include <functional>

#include "graphics/ui/widgets/IUiBase.h"

// Author: Claude
// Description: ImGui::Image / ImGui::ImageButton wrapper. onClick 콜백 유무로 두 호출을 분기한다.
// Input: 생성자 - textureHandle(백엔드별 텍스처를 가리키는 opaque 포인터), width/height(그릴 크기),
//        onClick(주어지면 ImageButton, 없으면 단순 Image로 그린다)
// Output: (해당 없음 - IUiBase 구현)
// Notes: textureHandle을 void*로 받는다 - ImGui의 ImTextureRef가 void*를 받는 암시적 생성자를
//        제공해서(imgui.h 참고), 이 헤더가 imgui.h를 포함하지 않고도 텍스처를 표현할 수 있다.
//        imgui.h/ImTextureRef로의 변환은 .cpp 안에서만 일어난다 - 이 헤더만 보고 ImageUI를
//        상속/사용하는 다른 모듈(WOT 포함)이 imgui.h를 몰라도 되게 하는 경계 유지 목적이다.
//        텍스처 로딩(파일 -> 포인터 변환)은 이번 요청 범위 밖이다 - 호출자가 이미 유효한 백엔드
//        텍스처 핸들을 들고 있다고 가정한다.
// Date: 2026-08-19
class ImageUI final : public IUiBase
{
public:
    ImageUI(void* textureHandle, float width, float height, std::function<void()> onClick = nullptr);

    void Render() override;

private:
    void* m_textureHandle;
    float m_width;
    float m_height;
    std::function<void()> m_onClick;
};
