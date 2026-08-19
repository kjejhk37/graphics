#include "graphics/renderer/directx9/DirectX9Renderer.h"

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
    D3DPRESENT_PARAMETERS MakePresentParams(HWND windowHandle, int width, int height)
    {
        D3DPRESENT_PARAMETERS presentParams{};
        presentParams.Windowed = TRUE;
        presentParams.SwapEffect = D3DSWAPEFFECT_DISCARD;
        presentParams.BackBufferFormat = D3DFMT_UNKNOWN;
        presentParams.hDeviceWindow = windowHandle;
        presentParams.BackBufferWidth = static_cast<UINT>(width);
        presentParams.BackBufferHeight = static_cast<UINT>(height);
        return presentParams;
    }

    constexpr const char* kCubeAssetPath = "assets/models/cube.obj";
    constexpr const char* kCubeCacheKey = "cube";
}

DirectX9Renderer::DirectX9Renderer(bool forceInstancingFallback) : m_forceInstancingFallback(forceInstancingFallback)
{
}

bool DirectX9Renderer::Initialize(HWND windowHandle, int width, int height)
{
    m_windowHandle = windowHandle;

    m_direct3D.Attach(Direct3DCreate9(D3D_SDK_VERSION));
    if (!m_direct3D)
    {
        return false;
    }

    D3DPRESENT_PARAMETERS presentParams = MakePresentParams(windowHandle, width, height);
    const HRESULT hr = m_direct3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, windowHandle,
                                                 D3DCREATE_HARDWARE_VERTEXPROCESSING, &presentParams,
                                                 m_device.GetAddressOf());
    if (FAILED(hr))
    {
        return false;
    }

    m_uiManager = std::make_unique<ImGuiManagerDX9>(windowHandle, m_device.Get());

    if (!CreateBaselinePipeline() || !CreateInstancingPipeline())
    {
        return false;
    }

    return true;
}

bool DirectX9Renderer::CreateBaselinePipeline()
{
    const std::vector<uint8_t> vsBytecode = ShaderBytecodeLoader::Load("shaders/directx9/Baseline.vs.cso");
    const std::vector<uint8_t> psBytecode = ShaderBytecodeLoader::Load("shaders/directx9/Baseline.ps.cso");

    if (FAILED(m_device->CreateVertexShader(reinterpret_cast<const DWORD*>(vsBytecode.data()),
                                             m_vertexShader.GetAddressOf())))
    {
        return false;
    }
    if (FAILED(m_device->CreatePixelShader(reinterpret_cast<const DWORD*>(psBytecode.data()),
                                            m_pixelShader.GetAddressOf())))
    {
        return false;
    }

    // NORMAL은 이번 사이클 셰이더가 실제로 읽지 않지만(라이팅 범위 밖), ModelMesh의 실제 정점
    // 레이아웃(position + normal)을 그대로 반영하기 위해 스트림 1로 선언해 둔다.
    static const D3DVERTEXELEMENT9 kVertexElements[] = {
        {0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {1, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0}, D3DDECL_END()};
    if (FAILED(m_device->CreateVertexDeclaration(kVertexElements, m_vertexDeclaration.GetAddressOf())))
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

    m_vertexCount = static_cast<UINT>(mesh.positions.size());
    const UINT positionBytes = static_cast<UINT>(mesh.positions.size() * sizeof(Vec3));
    // D3DPOOL_MANAGED: 정적(한 번만 채우는) 버퍼라 디바이스 Reset() 시에도 런타임이 자동 복원해준다 -
    // D3DPOOL_DEFAULT처럼 리사이즈/전체화면 전환마다 수동 재생성할 필요가 없다.
    if (FAILED(m_device->CreateVertexBuffer(positionBytes, 0, 0, D3DPOOL_MANAGED, m_positionBuffer.GetAddressOf(),
                                             nullptr)))
    {
        return false;
    }
    void* positionData = nullptr;
    if (FAILED(m_positionBuffer->Lock(0, positionBytes, &positionData, 0)))
    {
        return false;
    }
    std::memcpy(positionData, mesh.positions.data(), positionBytes);
    m_positionBuffer->Unlock();

    const UINT normalBytes = static_cast<UINT>(mesh.normals.size() * sizeof(Vec3));
    if (FAILED(
            m_device->CreateVertexBuffer(normalBytes, 0, 0, D3DPOOL_MANAGED, m_normalBuffer.GetAddressOf(), nullptr)))
    {
        return false;
    }
    void* normalData = nullptr;
    if (FAILED(m_normalBuffer->Lock(0, normalBytes, &normalData, 0)))
    {
        return false;
    }
    std::memcpy(normalData, mesh.normals.data(), normalBytes);
    m_normalBuffer->Unlock();

    m_indexCount = static_cast<UINT>(mesh.indices.size());
    const UINT indexBytes = m_indexCount * sizeof(uint32_t);
    if (FAILED(m_device->CreateIndexBuffer(indexBytes, 0, D3DFMT_INDEX32, D3DPOOL_MANAGED,
                                            m_indexBuffer.GetAddressOf(), nullptr)))
    {
        return false;
    }
    void* indexData = nullptr;
    if (FAILED(m_indexBuffer->Lock(0, indexBytes, &indexData, 0)))
    {
        return false;
    }
    std::memcpy(indexData, mesh.indices.data(), indexBytes);
    m_indexBuffer->Unlock();

    return true;
}

bool DirectX9Renderer::CreateInstancingPipeline()
{
    const std::vector<uint8_t> vsBytecode = ShaderBytecodeLoader::Load("shaders/directx9/Instancing.vs.cso");
    if (FAILED(m_device->CreateVertexShader(reinterpret_cast<const DWORD*>(vsBytecode.data()),
                                             m_instancingVertexShader.GetAddressOf())))
    {
        return false;
    }

    // 스트림 0 = 정점(POSITION, per-vertex), 스트림 1 = 인스턴스 월드 행렬(TEXCOORD1~4, per-instance).
    // Direct3D 9엔 4x4 행렬을 단일 세맨틱으로 선언하는 문법이 없어 4개의 float4로 쪼갠다.
    static const D3DVERTEXELEMENT9 kInstancingVertexElements[] = {
        {0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {1, 0, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 1},
        {1, 16, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 2},
        {1, 32, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 3},
        {1, 48, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 4}, D3DDECL_END()};
    if (FAILED(m_device->CreateVertexDeclaration(kInstancingVertexElements,
                                                  m_instancingVertexDeclaration.GetAddressOf())))
    {
        return false;
    }

    return true;
}

bool DirectX9Renderer::EnsureInstanceBufferCapacity(UINT instanceCount)
{
    if (m_instanceBuffer && instanceCount <= m_instanceBufferCapacity)
    {
        return true;
    }

    // D3DUSAGE_WRITEONLY를 일부러 빼둔다 - DebugReadBackInstanceBuffer(테스트 전용)가 Lock으로
    // 읽어야 하기 때문. D3DUSAGE_DYNAMIC만으로도 매 프레임 갱신하는 용도로는 충분하다.
    Microsoft::WRL::ComPtr<IDirect3DVertexBuffer9> newInstanceBuffer;
    const UINT bytes = instanceCount * sizeof(Matrix4x4);
    if (FAILED(m_device->CreateVertexBuffer(bytes, D3DUSAGE_DYNAMIC, 0, D3DPOOL_DEFAULT,
                                             newInstanceBuffer.GetAddressOf(), nullptr)))
    {
        return false;
    }
    m_instanceBuffer = newInstanceBuffer;
    m_instanceBufferCapacity = instanceCount;
    return true;
}

void DirectX9Renderer::DrawInstances(const std::vector<Matrix4x4>& worldMatrices)
{
    const UINT instanceCount = static_cast<UINT>(worldMatrices.size());
    bool hardwareInstancingSucceeded = false;

    if (!m_forceInstancingFallback && m_instancingVertexShader && m_instancingVertexDeclaration &&
        EnsureInstanceBufferCapacity(instanceCount))
    {
        void* instanceData = nullptr;
        const UINT bytes = instanceCount * sizeof(Matrix4x4);
        if (SUCCEEDED(m_instanceBuffer->Lock(0, bytes, &instanceData, D3DLOCK_DISCARD)))
        {
            std::memcpy(instanceData, worldMatrices.data(), bytes);
            m_instanceBuffer->Unlock();

            // Direct3D 9엔 인스턴싱 지원 여부를 사전에 질의하는 캡ability 플래그가 없다 - 아래
            // 호출들의 반환값으로만 성공 여부를 판단할 수 있다(체크리스트 8 요구사항).
            const HRESULT streamHr0 = m_device->SetStreamSource(0, m_positionBuffer.Get(), 0, sizeof(float) * 3);
            const HRESULT freqHr0 = m_device->SetStreamSourceFreq(0, D3DSTREAMSOURCE_INDEXEDDATA | instanceCount);
            const HRESULT streamHr1 = m_device->SetStreamSource(1, m_instanceBuffer.Get(), 0, sizeof(Matrix4x4));
            const HRESULT freqHr1 = m_device->SetStreamSourceFreq(1, D3DSTREAMSOURCE_INSTANCEDATA | 1);

            if (SUCCEEDED(streamHr0) && SUCCEEDED(freqHr0) && SUCCEEDED(streamHr1) && SUCCEEDED(freqHr1))
            {
                m_device->SetVertexDeclaration(m_instancingVertexDeclaration.Get());
                m_device->SetVertexShader(m_instancingVertexShader.Get());
                m_device->SetPixelShader(m_pixelShader.Get());
                m_device->SetIndices(m_indexBuffer.Get());
                const HRESULT drawHr =
                    m_device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, m_vertexCount, 0, m_indexCount / 3);
                hardwareInstancingSucceeded = SUCCEEDED(drawHr);
            }

            // 스트림 주파수를 기본값(1)으로 되돌려 이후(폴백 경로/다음 프레임) 드로우에 영향이 없게 한다.
            m_device->SetStreamSourceFreq(0, 1);
            m_device->SetStreamSourceFreq(1, 1);
        }
    }

    if (!hardwareInstancingSucceeded)
    {
        // 폴백: Baseline 파이프라인(단일 드로우용)을 인스턴스 개수만큼 반복 호출한다 - 별도 셰이더
        // 없이 c0 상수(WorldViewProj)만 매 인스턴스 세계 행렬로 갈아끼운다.
        m_device->SetVertexDeclaration(m_vertexDeclaration.Get());
        m_device->SetVertexShader(m_vertexShader.Get());
        m_device->SetPixelShader(m_pixelShader.Get());
        m_device->SetStreamSource(0, m_positionBuffer.Get(), 0, sizeof(float) * 3);
        m_device->SetStreamSource(1, m_normalBuffer.Get(), 0, sizeof(float) * 3);
        m_device->SetIndices(m_indexBuffer.Get());
        for (const Matrix4x4& worldMatrix : worldMatrices)
        {
            m_device->SetVertexShaderConstantF(0, reinterpret_cast<const float*>(&worldMatrix), 4);
            m_device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, m_vertexCount, 0, m_indexCount / 3);
        }
    }
}

void DirectX9Renderer::RenderFrame(const InstanceSnapshot& snapshot)
{
    if (!m_device)
    {
        return;
    }

    m_uiManager->NewFrame();

    if (m_uiElementRegistry)
    {
        m_uiElementRegistry->RenderAll();
    }

    m_device->Clear(0, nullptr, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
    m_device->BeginScene();

    if (!snapshot.worldMatrices.empty() && m_positionBuffer && m_indexBuffer)
    {
        DrawInstances(snapshot.worldMatrices);
    }
    else if (m_vertexShader && m_pixelShader && m_vertexDeclaration && m_positionBuffer && m_normalBuffer &&
             m_indexBuffer)
    {
        const Matrix4x4 worldViewProj = Matrix4x4::Identity();
        m_device->SetVertexShaderConstantF(0, reinterpret_cast<const float*>(&worldViewProj), 4);
        m_device->SetVertexDeclaration(m_vertexDeclaration.Get());
        m_device->SetVertexShader(m_vertexShader.Get());
        m_device->SetPixelShader(m_pixelShader.Get());
        m_device->SetStreamSource(0, m_positionBuffer.Get(), 0, sizeof(float) * 3);
        m_device->SetStreamSource(1, m_normalBuffer.Get(), 0, sizeof(float) * 3);
        m_device->SetIndices(m_indexBuffer.Get());
        const UINT triangleCount = m_indexCount / 3;
        m_device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, m_vertexCount, 0, triangleCount);
    }

    m_uiManager->Render();
    m_device->EndScene();
    m_device->Present(nullptr, nullptr, nullptr, nullptr);
}

bool DirectX9Renderer::DebugReadBackInstanceBuffer(std::vector<Matrix4x4>& outMatrices) const
{
    if (!m_instanceBuffer || m_instanceBufferCapacity == 0)
    {
        return false;
    }

    void* data = nullptr;
    const UINT bytes = m_instanceBufferCapacity * sizeof(Matrix4x4);
    if (FAILED(m_instanceBuffer->Lock(0, bytes, &data, D3DLOCK_READONLY)))
    {
        return false;
    }
    outMatrices.resize(m_instanceBufferCapacity);
    std::memcpy(outMatrices.data(), data, bytes);
    m_instanceBuffer->Unlock();
    return true;
}

void DirectX9Renderer::OnResize(int width, int height)
{
    if (!m_device || width <= 0 || height <= 0)
    {
        return;
    }

    D3DPRESENT_PARAMETERS presentParams = MakePresentParams(m_windowHandle, width, height);
    m_device->Reset(&presentParams);
}

void DirectX9Renderer::Shutdown()
{
    if (m_uiManager)
    {
        m_uiManager->Shutdown();
        m_uiManager.reset();
    }
    m_instanceBuffer.Reset();
    m_instancingVertexDeclaration.Reset();
    m_instancingVertexShader.Reset();
    m_indexBuffer.Reset();
    m_normalBuffer.Reset();
    m_positionBuffer.Reset();
    m_vertexDeclaration.Reset();
    m_pixelShader.Reset();
    m_vertexShader.Reset();
    m_device.Reset();
    m_direct3D.Reset();
}

bool DirectX9Renderer::HandleUiMessage(HWND windowHandle, UINT message, WPARAM wParam, LPARAM lParam)
{
    return m_uiManager && m_uiManager->HandleWin32Message(windowHandle, message, wParam, lParam);
}

void DirectX9Renderer::SetUiElementRegistry(IUiElementRegistry& registry)
{
    m_uiElementRegistry = &registry;
}

void* DirectX9Renderer::LoadTexture(const std::string& /*filePath*/)
{
    return nullptr;
}

void DirectX9Renderer::UnloadTexture(void* /*textureHandle*/)
{
}
