#pragma once

#include <memory>

#include "graphics/renderer/IRenderer.h"
#include "graphics/renderer/RendererBackend.h"

// Author: Claude
// Description: RendererBackend에 따라 알맞은 IRenderer 구현체를 생성한다.
// Input: 선택된 RendererBackend
// Output: 선택된 백엔드에 대응하는 IRenderer 구현체의 소유권 있는 포인터
// Notes: 새 백엔드가 추가되면 이 팩토리에만 분기를 추가하면 되고, 호출부(main)는 변경할 필요가 없다.
//        LaunchConfig(projects 소속)를 직접 받지 않는다 — graphics가 projects의 설정 타입을 알면
//        안 된다는 계층 원칙 때문(docs/architecture/platform_graphics_projects.md 참고). 호출부가
//        LaunchConfig에서 backend 필드만 꺼내 넘긴다.
// Date: 2026-07-15
namespace RendererFactory
{
    std::unique_ptr<IRenderer> Create(RendererBackend backend);
}
