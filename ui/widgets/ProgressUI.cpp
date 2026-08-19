#include "graphics/ui/widgets/ProgressUI.h"

#include <imgui.h>

#include <cfloat>
#include <utility>

ProgressUI::ProgressUI(const float* fraction, std::string overlayLabel)
    : m_fraction(fraction)
    , m_overlayLabel(std::move(overlayLabel))
{
}

void ProgressUI::Render()
{
    const float fraction = m_fraction ? *m_fraction : 0.0f;
    ImGui::ProgressBar(fraction, ImVec2(-FLT_MIN, 0.0f), m_overlayLabel.empty() ? nullptr : m_overlayLabel.c_str());
}
