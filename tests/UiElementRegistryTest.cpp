#include <gtest/gtest.h>

#include "graphics/ui/widgets/IUiBase.h"
#include "graphics/ui/widgets/UiElementRegistry.h"

namespace
{
    class MockUiBase final : public IUiBase
    {
    public:
        void Render() override { ++renderCount; }

        int renderCount = 0;
    };

    class SelfRemovingUiBase final : public IUiBase
    {
    public:
        explicit SelfRemovingUiBase(UiElementRegistry& registry) : m_registry(registry) {}

        void Render() override
        {
            ++renderCount;
            m_registry.Remove(this);
        }

        int renderCount = 0;

    private:
        UiElementRegistry& m_registry;
    };
}

TEST(UiElementRegistryTest, RenderAllCallsRenderOnEveryRegisteredWidget)
{
    UiElementRegistry registry;
    MockUiBase widgetA;
    MockUiBase widgetB;

    registry.Add(&widgetA);
    registry.Add(&widgetB);
    registry.RenderAll();

    EXPECT_EQ(widgetA.renderCount, 1);
    EXPECT_EQ(widgetB.renderCount, 1);
}

TEST(UiElementRegistryTest, RemoveExcludesWidgetFromRenderAll)
{
    UiElementRegistry registry;
    MockUiBase widgetA;
    MockUiBase widgetB;

    registry.Add(&widgetA);
    registry.Add(&widgetB);
    registry.Remove(&widgetA);
    registry.RenderAll();

    EXPECT_EQ(widgetA.renderCount, 0);
    EXPECT_EQ(widgetB.renderCount, 1);
}

TEST(UiElementRegistryTest, AddIgnoresNullptr)
{
    UiElementRegistry registry;
    registry.Add(nullptr);

    EXPECT_NO_FATAL_FAILURE(registry.RenderAll());
}

TEST(UiElementRegistryTest, RemoveNonRegisteredWidgetDoesNotCrash)
{
    UiElementRegistry registry;
    MockUiBase widgetA;

    EXPECT_NO_FATAL_FAILURE(registry.Remove(&widgetA));
}

// 위젯이 자신의 Render() 안에서 스스로를 registry에서 제거하는 것은 실사용에서 나올 수 있는
// 패턴이다(예: "닫기" 버튼) - RenderAll()이 라이브 컨테이너를 순회하면 이 시나리오에서 순회 중
// 컨테이너가 변경돼 undefined behavior가 된다.
TEST(UiElementRegistryTest, RenderAllToleratesWidgetRemovingItselfDuringRender)
{
    UiElementRegistry registry;
    SelfRemovingUiBase widgetA(registry);
    MockUiBase widgetB;

    registry.Add(&widgetA);
    registry.Add(&widgetB);

    EXPECT_NO_FATAL_FAILURE(registry.RenderAll());

    EXPECT_EQ(widgetA.renderCount, 1);
    EXPECT_EQ(widgetB.renderCount, 1);
}
