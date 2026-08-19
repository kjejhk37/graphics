#include "graphics/ui/widgets/LabelUI.h"

#include <imgui.h>

#include <utility>

LabelUI::LabelUI(std::string text)
    : m_text(std::move(text))
{
}

void LabelUI::SetText(std::string text)
{
    m_text = std::move(text);
}

void LabelUI::Render()
{
    ImGui::TextUnformatted(m_text.c_str());
}
