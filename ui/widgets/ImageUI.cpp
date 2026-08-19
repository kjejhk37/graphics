#include "graphics/ui/widgets/ImageUI.h"

#include <imgui.h>

#include <utility>

ImageUI::ImageUI(void* textureHandle, float width, float height, std::function<void()> onClick)
    : m_textureHandle(textureHandle)
    , m_width(width)
    , m_height(height)
    , m_onClick(std::move(onClick))
{
}

void ImageUI::Render()
{
    const ImVec2 size(m_width, m_height);

    // this 포인터로 ID를 고정해, 서로 다른 ImageUI 인스턴스가 같은 문자열 ID("##image")를
    // 써도 ImGui ID 충돌이 나지 않게 한다(ButtonUI/StringFieldUI와 달리 사용자가 라벨을
    // 주지 않으므로 자체적으로 유일성을 보장해야 한다).
    ImGui::PushID(this);
    if (m_onClick)
    {
        if (ImGui::ImageButton("##image", m_textureHandle, size))
        {
            m_onClick();
        }
    }
    else
    {
        ImGui::Image(m_textureHandle, size);
    }
    ImGui::PopID();
}
