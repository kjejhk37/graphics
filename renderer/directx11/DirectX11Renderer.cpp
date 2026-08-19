#include "graphics/renderer/directx11/DirectX11Renderer.h"

#include <cstring>
#include <memory>

#include "platform/math/Matrix4x4.h"
#include "platform/model_import/MeshManager.h"
#include "platform/model_import/ModelLoader.h"
#include "platform/model_import/RefCountingCachePolicy.h"
#include "graphics/renderer/ShaderBytecodeLoader.h"
#include "graphics/ui/widgets/IUiElementRegistry.h"

namespace
{
    constexpr const char* kCubeAssetPath = "assets/models/cube.obj";
    constexpr const char* kCubeCacheKey = "cube";
}

DirectX11Renderer::DirectX11Renderer(bool forceWarp)
    : m_forceWarp(forceWarp)
{
}

bool DirectX11Renderer::CreateDeviceAndSwapChain(D3D_DRIVER_TYPE driverType, HWND windowHandle, int width, int height)
{
    DXGI_SWAP_CHAIN_DESC swapChainDesc{};
    swapChainDesc.BufferCount = 1;
    swapChainDesc.BufferDesc.Width = static_cast<UINT>(width);
    swapChainDesc.BufferDesc.Height = static_cast<UINT>(height);
    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.OutputWindow = windowHandle;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.Windowed = TRUE;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    const HRESULT hr =
        D3D11CreateDeviceAndSwapChain(nullptr, driverType, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION, &swapChainDesc,
                                       m_swapChain.GetAddressOf(), m_device.GetAddressOf(), nullptr,
                                       m_context.GetAddressOf());
    return SUCCEEDED(hr);
}

bool DirectX11Renderer::CreateRenderTargetView()
{
    Microsoft::WRL::ComPtr<ID3D11Texture2D> backBuffer;
    HRESULT hr = m_swapChain->GetBuffer(0, IID_PPV_ARGS(backBuffer.GetAddressOf()));
    if (FAILED(hr))
    {
        return false;
    }

    hr = m_device->CreateRenderTargetView(backBuffer.Get(), nullptr, m_renderTargetView.GetAddressOf());
    return SUCCEEDED(hr);
}

bool DirectX11Renderer::Initialize(HWND windowHandle, int width, int height)
{
    m_width = static_cast<UINT>(width);
    m_height = static_cast<UINT>(height);

    bool created = false;
    if (!m_forceWarp)
    {
        created = CreateDeviceAndSwapChain(D3D_DRIVER_TYPE_HARDWARE, windowHandle, width, height);
    }
    if (!created)
    {
        created = CreateDeviceAndSwapChain(D3D_DRIVER_TYPE_WARP, windowHandle, width, height);
    }
    if (!created)
    {
        return false;
    }

    if (!CreateRenderTargetView())
    {
        return false;
    }

    m_uiManager = std::make_unique<ImGuiManagerDX11>(windowHandle, m_device.Get(), m_context.Get());

    if (!CreateBaselinePipeline() || !CreateInstancingPipeline())
    {
        return false;
    }

    return true;
}

bool DirectX11Renderer::CreateBaselinePipeline()
{
    const std::vector<uint8_t> vsBytecode = ShaderBytecodeLoader::Load("shaders/directx11/Baseline.vs.cso");
    const std::vector<uint8_t> psBytecode = ShaderBytecodeLoader::Load("shaders/directx11/Baseline.ps.cso");

    if (FAILED(m_device->CreateVertexShader(vsBytecode.data(), vsBytecode.size(), nullptr,
                                             m_vertexShader.GetAddressOf())))
    {
        return false;
    }
    if (FAILED(
            m_device->CreatePixelShader(psBytecode.data(), psBytecode.size(), nullptr, m_pixelShader.GetAddressOf())))
    {
        return false;
    }

    // NORMAL은 이번 사이클 셰이더가 실제로 읽지 않지만(라이팅 범위 밖), ModelMesh의 실제 정점
    // 레이아웃(position + normal)을 그대로 반영하기 위해 입력 슬롯 1로 선언해 둔다.
    static const D3D11_INPUT_ELEMENT_DESC kInputElements[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 1, 0, D3D11_INPUT_PER_VERTEX_DATA, 0}};
    if (FAILED(m_device->CreateInputLayout(kInputElements, 2, vsBytecode.data(), vsBytecode.size(),
                                            m_inputLayout.GetAddressOf())))
    {
        return false;
    }

    MeshManager meshManager(std::make_unique<RefCountingCachePolicy>());
    const std::shared_ptr<const Model> model =
        meshManager.GetOrLoad(kCubeCacheKey, []() { return ModelLoader::LoadOBJ(kCubeAssetPath); });
    if (!model || model->meshes.empty())
    {
        return false;
    }
    const ModelMesh& mesh = *model->meshes[0];
    if (mesh.positions.empty() || mesh.normals.size() != mesh.positions.size() || mesh.indices.empty())
    {
        return false;
    }

    D3D11_BUFFER_DESC positionBufferDesc{};
    positionBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
    positionBufferDesc.ByteWidth = static_cast<UINT>(mesh.positions.size() * sizeof(Vec3));
    positionBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA positionInitData{};
    positionInitData.pSysMem = mesh.positions.data();
    if (FAILED(m_device->CreateBuffer(&positionBufferDesc, &positionInitData, m_positionBuffer.GetAddressOf())))
    {
        return false;
    }

    D3D11_BUFFER_DESC normalBufferDesc{};
    normalBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
    normalBufferDesc.ByteWidth = static_cast<UINT>(mesh.normals.size() * sizeof(Vec3));
    normalBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA normalInitData{};
    normalInitData.pSysMem = mesh.normals.data();
    if (FAILED(m_device->CreateBuffer(&normalBufferDesc, &normalInitData, m_normalBuffer.GetAddressOf())))
    {
        return false;
    }

    m_indexCount = static_cast<UINT>(mesh.indices.size());
    D3D11_BUFFER_DESC indexBufferDesc{};
    indexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
    indexBufferDesc.ByteWidth = static_cast<UINT>(mesh.indices.size() * sizeof(uint32_t));
    indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    D3D11_SUBRESOURCE_DATA indexInitData{};
    indexInitData.pSysMem = mesh.indices.data();
    if (FAILED(m_device->CreateBuffer(&indexBufferDesc, &indexInitData, m_indexBuffer.GetAddressOf())))
    {
        return false;
    }

    D3D11_BUFFER_DESC constantBufferDesc{};
    constantBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    constantBufferDesc.ByteWidth = sizeof(Matrix4x4);
    constantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    constantBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    if (FAILED(m_device->CreateBuffer(&constantBufferDesc, nullptr, m_constantBuffer.GetAddressOf())))
    {
        return false;
    }

    return true;
}

bool DirectX11Renderer::CreateInstancingPipeline()
{
    const std::vector<uint8_t> vsBytecode = ShaderBytecodeLoader::Load("shaders/directx11/Instancing.vs.cso");
    if (FAILED(m_device->CreateVertexShader(vsBytecode.data(), vsBytecode.size(), nullptr,
                                             m_instancingVertexShader.GetAddressOf())))
    {
        return false;
    }

    // INSTANCE_WORLD는 float4x4 하나를 TEXCOORD 스타일 4개의 float4 원소로 쪼개 표현하는 D3D의
    // 표준 관례다 - 각 원소는 입력 슬롯 1(인스턴스 버퍼)에서 인스턴스마다 한 번씩(InstanceDataStepRate=1) 읽힌다.
    static const D3D11_INPUT_ELEMENT_DESC kInstancingInputElements[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"INSTANCE_WORLD", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0, D3D11_INPUT_PER_INSTANCE_DATA, 1},
        {"INSTANCE_WORLD", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 16, D3D11_INPUT_PER_INSTANCE_DATA, 1},
        {"INSTANCE_WORLD", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 32, D3D11_INPUT_PER_INSTANCE_DATA, 1},
        {"INSTANCE_WORLD", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 48, D3D11_INPUT_PER_INSTANCE_DATA, 1},
    };
    if (FAILED(m_device->CreateInputLayout(kInstancingInputElements, 5, vsBytecode.data(), vsBytecode.size(),
                                            m_instancingInputLayout.GetAddressOf())))
    {
        return false;
    }

    return true;
}

bool DirectX11Renderer::EnsureInstanceBufferCapacity(UINT instanceCount)
{
    if (m_instanceBuffer && instanceCount <= m_instanceBufferCapacity)
    {
        return true;
    }

    D3D11_BUFFER_DESC instanceBufferDesc{};
    instanceBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    instanceBufferDesc.ByteWidth = instanceCount * sizeof(Matrix4x4);
    instanceBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    instanceBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    Microsoft::WRL::ComPtr<ID3D11Buffer> newInstanceBuffer;
    if (FAILED(m_device->CreateBuffer(&instanceBufferDesc, nullptr, newInstanceBuffer.GetAddressOf())))
    {
        return false;
    }
    m_instanceBuffer = newInstanceBuffer;
    m_instanceBufferCapacity = instanceCount;
    return true;
}

void DirectX11Renderer::RenderFrame(const InstanceSnapshot& snapshot)
{
    if (!m_context || !m_renderTargetView)
    {
        return;
    }

    m_uiManager->NewFrame();

    if (m_uiElementRegistry)
    {
        m_uiElementRegistry->RenderAll();
    }

    constexpr float kClearColor[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    m_context->OMSetRenderTargets(1, m_renderTargetView.GetAddressOf(), nullptr);
    m_context->ClearRenderTargetView(m_renderTargetView.Get(), kClearColor);

    const D3D11_VIEWPORT viewport{0.0f, 0.0f, static_cast<float>(m_width), static_cast<float>(m_height), 0.0f, 1.0f};
    m_context->RSSetViewports(1, &viewport);

    const bool canDrawInstanced = !snapshot.worldMatrices.empty() && m_instancingVertexShader &&
                                   m_instancingInputLayout && m_positionBuffer && m_indexBuffer && m_constantBuffer;
    if (canDrawInstanced)
    {
        const UINT instanceCount = static_cast<UINT>(snapshot.worldMatrices.size());
        if (EnsureInstanceBufferCapacity(instanceCount))
        {
            D3D11_MAPPED_SUBRESOURCE mappedInstances{};
            if (SUCCEEDED(m_context->Map(m_instanceBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedInstances)))
            {
                std::memcpy(mappedInstances.pData, snapshot.worldMatrices.data(), instanceCount * sizeof(Matrix4x4));
                m_context->Unmap(m_instanceBuffer.Get(), 0);
            }

            D3D11_MAPPED_SUBRESOURCE mappedViewProj{};
            if (SUCCEEDED(m_context->Map(m_constantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedViewProj)))
            {
                const Matrix4x4 viewProj = Matrix4x4::Identity();
                std::memcpy(mappedViewProj.pData, &viewProj, sizeof(Matrix4x4));
                m_context->Unmap(m_constantBuffer.Get(), 0);
            }

            ID3D11Buffer* vertexBuffers[] = {m_positionBuffer.Get(), m_instanceBuffer.Get()};
            const UINT strides[] = {sizeof(float) * 3, sizeof(Matrix4x4)};
            const UINT offsets[] = {0, 0};
            m_context->IASetInputLayout(m_instancingInputLayout.Get());
            m_context->IASetVertexBuffers(0, 2, vertexBuffers, strides, offsets);
            m_context->IASetIndexBuffer(m_indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
            m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            m_context->VSSetShader(m_instancingVertexShader.Get(), nullptr, 0);
            m_context->VSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());
            m_context->PSSetShader(m_pixelShader.Get(), nullptr, 0);
            m_context->DrawIndexedInstanced(m_indexCount, instanceCount, 0, 0, 0);
        }
    }
    else if (m_vertexShader && m_pixelShader && m_inputLayout && m_positionBuffer && m_normalBuffer && m_indexBuffer &&
             m_constantBuffer)
    {
        D3D11_MAPPED_SUBRESOURCE mapped{};
        if (SUCCEEDED(m_context->Map(m_constantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
        {
            const Matrix4x4 worldViewProj = Matrix4x4::Identity();
            std::memcpy(mapped.pData, &worldViewProj, sizeof(Matrix4x4));
            m_context->Unmap(m_constantBuffer.Get(), 0);
        }

        ID3D11Buffer* vertexBuffers[] = {m_positionBuffer.Get(), m_normalBuffer.Get()};
        const UINT strides[] = {sizeof(float) * 3, sizeof(float) * 3};
        const UINT offsets[] = {0, 0};
        m_context->IASetInputLayout(m_inputLayout.Get());
        m_context->IASetVertexBuffers(0, 2, vertexBuffers, strides, offsets);
        m_context->IASetIndexBuffer(m_indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
        m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        m_context->VSSetShader(m_vertexShader.Get(), nullptr, 0);
        m_context->VSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());
        m_context->PSSetShader(m_pixelShader.Get(), nullptr, 0);
        m_context->DrawIndexed(m_indexCount, 0, 0);
    }

    m_uiManager->Render();
    m_swapChain->Present(1, 0);
}

bool DirectX11Renderer::DebugReadBackInstanceBuffer(std::vector<Matrix4x4>& outMatrices) const
{
    if (!m_instanceBuffer || m_instanceBufferCapacity == 0)
    {
        return false;
    }

    D3D11_BUFFER_DESC stagingDesc{};
    stagingDesc.Usage = D3D11_USAGE_STAGING;
    stagingDesc.ByteWidth = m_instanceBufferCapacity * sizeof(Matrix4x4);
    stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

    Microsoft::WRL::ComPtr<ID3D11Buffer> stagingBuffer;
    if (FAILED(m_device->CreateBuffer(&stagingDesc, nullptr, stagingBuffer.GetAddressOf())))
    {
        return false;
    }
    m_context->CopyResource(stagingBuffer.Get(), m_instanceBuffer.Get());

    D3D11_MAPPED_SUBRESOURCE mapped{};
    if (FAILED(m_context->Map(stagingBuffer.Get(), 0, D3D11_MAP_READ, 0, &mapped)))
    {
        return false;
    }
    outMatrices.resize(m_instanceBufferCapacity);
    std::memcpy(outMatrices.data(), mapped.pData, m_instanceBufferCapacity * sizeof(Matrix4x4));
    m_context->Unmap(stagingBuffer.Get(), 0);
    return true;
}

void DirectX11Renderer::OnResize(int width, int height)
{
    if (!m_swapChain || width <= 0 || height <= 0)
    {
        return;
    }

    m_width = static_cast<UINT>(width);
    m_height = static_cast<UINT>(height);

    m_context->OMSetRenderTargets(0, nullptr, nullptr);
    m_renderTargetView.Reset();

    const HRESULT hr =
        m_swapChain->ResizeBuffers(0, static_cast<UINT>(width), static_cast<UINT>(height), DXGI_FORMAT_UNKNOWN, 0);
    if (SUCCEEDED(hr))
    {
        CreateRenderTargetView();
    }
}

void DirectX11Renderer::Shutdown()
{
    if (m_uiManager)
    {
        m_uiManager->Shutdown();
        m_uiManager.reset();
    }
    m_instanceBuffer.Reset();
    m_instancingInputLayout.Reset();
    m_instancingVertexShader.Reset();
    m_constantBuffer.Reset();
    m_indexBuffer.Reset();
    m_normalBuffer.Reset();
    m_positionBuffer.Reset();
    m_inputLayout.Reset();
    m_pixelShader.Reset();
    m_vertexShader.Reset();
    m_renderTargetView.Reset();
    m_swapChain.Reset();
    m_context.Reset();
    m_device.Reset();
}

bool DirectX11Renderer::HandleUiMessage(HWND windowHandle, UINT message, WPARAM wParam, LPARAM lParam)
{
    return m_uiManager && m_uiManager->HandleWin32Message(windowHandle, message, wParam, lParam);
}

void DirectX11Renderer::SetUiElementRegistry(IUiElementRegistry& registry)
{
    m_uiElementRegistry = &registry;
}
