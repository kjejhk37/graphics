#include "graphics/renderer/RendererFactory.h"

#include "graphics/renderer/directx11/DirectX11Renderer.h"
#include "graphics/renderer/directx12/DirectX12Renderer.h"
#include "graphics/renderer/directx9/DirectX9Renderer.h"
#include "graphics/renderer/opengl/OpenGLRenderer.h"

namespace RendererFactory
{
    std::unique_ptr<IRenderer> Create(RendererBackend backend)
    {
        switch (backend)
        {
        case RendererBackend::DirectX9:
            return std::make_unique<DirectX9Renderer>();
        case RendererBackend::DirectX11:
            return std::make_unique<DirectX11Renderer>();
        case RendererBackend::DirectX12:
            return std::make_unique<DirectX12Renderer>();
        case RendererBackend::OpenGL:
            return std::make_unique<OpenGLRenderer>();
        }
        return nullptr;
    }
}
