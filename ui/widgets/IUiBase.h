#pragma once

// Author: Claude
// Description: 위젯 하나가 자기 자신을 그릴 수 있다는 것만 보장하는 최소 렌더링 계약.
//              구체 위젯(ButtonUI 등)이 어떤 ImGui 호출을 하는지는 이 인터페이스가 알 필요가 없다.
// Input: (해당 없음 - 인터페이스)
// Output: (해당 없음 - 인터페이스)
// Notes: Render()는 IUiManager::NewFrame() 이후, IUiManager::Render() 이전(활성 ImGui 프레임 안)에
//        호출되는 것을 전제로 한다 - 그 밖에서 호출하면 ImGui가 assert로 죽는다.
// Date: 2026-08-19
class IUiBase
{
public:
    virtual ~IUiBase() = default;

    virtual void Render() = 0;
};
