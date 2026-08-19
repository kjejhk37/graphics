#include "graphics/ui/widgets/StringFieldUI.h"

#include <imgui.h>

#include <algorithm>
#include <cstring>
#include <utility>

StringFieldUI::StringFieldUI(std::string label, std::string* buffer, std::size_t bufferCapacity)
    : m_label(std::move(label))
    , m_buffer(buffer)
    , m_editBuffer(bufferCapacity > 0 ? bufferCapacity : 1, '\0')
{
}

void StringFieldUI::Render()
{
    if (!m_buffer)
    {
        return;
    }

    const std::size_t copyLength = std::min(m_buffer->size(), m_editBuffer.size() - 1);
    std::memcpy(m_editBuffer.data(), m_buffer->data(), copyLength);
    m_editBuffer[copyLength] = '\0';

    if (ImGui::InputText(m_label.c_str(), m_editBuffer.data(), m_editBuffer.size()))
    {
        *m_buffer = m_editBuffer.data();
    }
}
