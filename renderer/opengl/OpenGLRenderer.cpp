#include "graphics/renderer/opengl/OpenGLRenderer.h"

bool OpenGLRenderer::Initialize(HWND /*windowHandle*/, int /*width*/, int /*height*/)
{
    return true;
}

void OpenGLRenderer::RenderFrame(const InstanceSnapshot& /*snapshot*/)
{
}

void OpenGLRenderer::OnResize(int /*width*/, int /*height*/)
{
}

void OpenGLRenderer::Shutdown()
{
}

bool OpenGLRenderer::HandleUiMessage(HWND /*windowHandle*/, UINT /*message*/, WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    return false;
}

void OpenGLRenderer::SetUiElementRegistry(IUiElementRegistry& /*registry*/)
{
}

void* OpenGLRenderer::LoadTexture(const std::string& /*filePath*/)
{
    return nullptr;
}

void OpenGLRenderer::UnloadTexture(void* /*textureHandle*/)
{
}
