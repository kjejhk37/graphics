#pragma once

#include <optional>
#include <string_view>

#include "graphics/renderer/RendererBackend.h"

// Author: Claude
// Description: 문자열을 RendererBackend로 변환하는 공용 파서. LaunchConfigParser(argv)와 ConfigManager(JSON)가 함께 사용한다.
// Input: "directx" 또는 "opengl" 같은 백엔드 이름 문자열
// Output: 인식된 값이면 해당 RendererBackend, 아니면 std::nullopt
// Notes: 새 백엔드가 추가되면 이 파서에만 분기를 추가하면 되고, 호출부(argv 파서, JSON Config 파서)는 변경할 필요가 없다.
// Date: 2026-07-19
namespace RendererBackendParser
{
    std::optional<RendererBackend> TryParse(std::string_view value);
}
