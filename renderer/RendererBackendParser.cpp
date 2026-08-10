#include "graphics/renderer/RendererBackendParser.h"

// 로컬 상수 분리 기준: 이 파일이 500줄을 넘거나 아래 상수가 5개를 넘으면
// RendererBackendNames.h 같은 별도 헤더로 분리한다.
namespace
{
    constexpr std::string_view kDirectX9Name = "directx9";
    constexpr std::string_view kDirectX11Name = "directx11";
    constexpr std::string_view kDirectX12Name = "directx12";
    constexpr std::string_view kOpenGLName = "opengl";
}

namespace RendererBackendParser
{
    std::optional<RendererBackend> TryParse(std::string_view value)
    {
        if (value == kDirectX9Name)
        {
            return RendererBackend::DirectX9;
        }
        if (value == kDirectX11Name)
        {
            return RendererBackend::DirectX11;
        }
        if (value == kDirectX12Name)
        {
            return RendererBackend::DirectX12;
        }
        if (value == kOpenGLName)
        {
            return RendererBackend::OpenGL;
        }
        return std::nullopt;
    }
}
