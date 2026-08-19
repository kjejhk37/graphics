#include "graphics/ui/widgets/ButtonUI.h"

#include <imgui.h>

#include <utility>

ButtonUI::ButtonUI(std::string label, std::function<void()> onClick)
    : m_label(std::move(label))
    , m_onClick(std::move(onClick))
{
}

void ButtonUI::Render()
{
    if (ImGui::Button(m_label.c_str()) && m_onClick)
    {
        m_onClick();
    }
}
