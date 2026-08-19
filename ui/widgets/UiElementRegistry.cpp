#include "graphics/ui/widgets/UiElementRegistry.h"

#include <algorithm>

#include "graphics/ui/widgets/IUiBase.h"

void UiElementRegistry::Add(IUiBase* widget)
{
    if (!widget)
    {
        return;
    }
    m_widgets.push_back(widget);
}

void UiElementRegistry::Remove(IUiBase* widget)
{
    m_widgets.erase(std::remove(m_widgets.begin(), m_widgets.end(), widget), m_widgets.end());
}

void UiElementRegistry::RenderAll()
{
    // m_widgets를 직접 순회하지 않고 스냅샷 복사본 위에서 순회한다 - 위젯의 Render()(클릭 콜백 등)가
    // 자기 자신이나 다른 위젯을 registry에서 Add/Remove하면 라이브 벡터가 변경돼(재할당/erase)
    // 순회 중인 iterator가 무효화되기 때문이다(자기 자신을 제거하는 위젯 패턴에서 실제로 발생 가능).
    const std::vector<IUiBase*> widgetsSnapshot = m_widgets;
    for (IUiBase* widget : widgetsSnapshot)
    {
        widget->Render();
    }
}
