#include "graphics/renderer/directx12/DirectX12Renderer.h"

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

    Microsoft::WRL::ComPtr<ID3D12Resource> CreateUploadBuffer(ID3D12Device* device, UINT64 sizeInBytes)
    {
        D3D12_HEAP_PROPERTIES heapProperties{};
        heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;

        D3D12_RESOURCE_DESC resourceDesc{};
        resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        resourceDesc.Width = sizeInBytes;
        resourceDesc.Height = 1;
        resourceDesc.DepthOrArraySize = 1;
        resourceDesc.MipLevels = 1;
        resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
        resourceDesc.SampleDesc.Count = 1;
        resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

        Microsoft::WRL::ComPtr<ID3D12Resource> resource;
        device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc,
                                         D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&resource));
        return resource;
    }

    // 업로드 힙 리소스를 만들고 CPU에서 곧바로 데이터를 채운 뒤 매핑을 해제한다 - 정점/인덱스처럼
    // 한 번만 채우고 그 뒤로는 GPU만 읽는 정적 데이터에 쓴다(상수 버퍼처럼 매 프레임 갱신하는
    // 데이터는 이 헬퍼를 쓰지 않고 매핑을 유지한다).
    Microsoft::WRL::ComPtr<ID3D12Resource> CreateUploadBufferWithData(ID3D12Device* device, const void* data,
                                                                       UINT64 sizeInBytes)
    {
        Microsoft::WRL::ComPtr<ID3D12Resource> resource = CreateUploadBuffer(device, sizeInBytes);
        if (!resource)
        {
            return resource;
        }

        void* mappedData = nullptr;
        const D3D12_RANGE noReadRange{0, 0};
        if (FAILED(resource->Map(0, &noReadRange, &mappedData)))
        {
            return nullptr;
        }
        std::memcpy(mappedData, data, sizeInBytes);
        resource->Unmap(0, nullptr);
        return resource;
    }
}

DirectX12Renderer::DirectX12Renderer(bool forceWarp)
    : m_forceWarp(forceWarp)
{
}

DirectX12Renderer::~DirectX12Renderer()
{
    Shutdown();
}

bool DirectX12Renderer::CreateDevice()
{
    if (!m_forceWarp &&
        SUCCEEDED(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(m_device.GetAddressOf()))))
    {
        return true;
    }

    Microsoft::WRL::ComPtr<IDXGIFactory4> factory;
    if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(factory.GetAddressOf()))))
    {
        return false;
    }

    Microsoft::WRL::ComPtr<IDXGIAdapter> warpAdapter;
    if (FAILED(factory->EnumWarpAdapter(IID_PPV_ARGS(warpAdapter.GetAddressOf()))))
    {
        return false;
    }

    return SUCCEEDED(
        D3D12CreateDevice(warpAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(m_device.GetAddressOf())));
}

bool DirectX12Renderer::CreateSwapChain(HWND windowHandle, int width, int height)
{
    D3D12_COMMAND_QUEUE_DESC queueDesc{};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    if (FAILED(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(m_commandQueue.GetAddressOf()))))
    {
        return false;
    }

    Microsoft::WRL::ComPtr<IDXGIFactory4> factory;
    if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(factory.GetAddressOf()))))
    {
        return false;
    }

    DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
    swapChainDesc.BufferCount = kBackBufferCount;
    swapChainDesc.Width = static_cast<UINT>(width);
    swapChainDesc.Height = static_cast<UINT>(height);
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.SampleDesc.Count = 1;

    Microsoft::WRL::ComPtr<IDXGISwapChain1> swapChain1;
    if (FAILED(factory->CreateSwapChainForHwnd(m_commandQueue.Get(), windowHandle, &swapChainDesc, nullptr, nullptr,
                                                swapChain1.GetAddressOf())))
    {
        return false;
    }

    if (FAILED(swapChain1.As(&m_swapChain)))
    {
        return false;
    }

    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
    return true;
}

bool DirectX12Renderer::CreateRenderTargets()
{
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};
    heapDesc.NumDescriptors = kBackBufferCount;
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    if (FAILED(m_device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(m_rtvHeap.GetAddressOf()))))
    {
        return false;
    }

    m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
    for (UINT i = 0; i < kBackBufferCount; ++i)
    {
        if (FAILED(m_swapChain->GetBuffer(i, IID_PPV_ARGS(m_renderTargets[i].GetAddressOf()))))
        {
            return false;
        }
        m_device->CreateRenderTargetView(m_renderTargets[i].Get(), nullptr, rtvHandle);
        rtvHandle.ptr += m_rtvDescriptorSize;
    }
    return true;
}

void DirectX12Renderer::ReleaseRenderTargets()
{
    for (auto& renderTarget : m_renderTargets)
    {
        renderTarget.Reset();
    }
}

void DirectX12Renderer::WaitForGpu()
{
    if (!m_commandQueue || !m_fence)
    {
        return;
    }

    const UINT64 fenceValueToSignal = m_fenceValue;
    m_commandQueue->Signal(m_fence.Get(), fenceValueToSignal);
    ++m_fenceValue;

    if (m_fence->GetCompletedValue() < fenceValueToSignal)
    {
        m_fence->SetEventOnCompletion(fenceValueToSignal, m_fenceEvent);
        WaitForSingleObject(m_fenceEvent, INFINITE);
    }
}

bool DirectX12Renderer::Initialize(HWND windowHandle, int width, int height)
{
    m_width = static_cast<UINT>(width);
    m_height = static_cast<UINT>(height);

    if (!CreateDevice() || !CreateSwapChain(windowHandle, width, height) || !CreateRenderTargets())
    {
        return false;
    }

    for (UINT i = 0; i < kCommandListCount; ++i)
    {
        if (FAILED(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
                                                      IID_PPV_ARGS(m_commandAllocators[i].GetAddressOf()))))
        {
            return false;
        }
    }

    if (FAILED(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocators[0].Get(), nullptr,
                                            IID_PPV_ARGS(m_commandLists[0].GetAddressOf()))))
    {
        return false;
    }
    m_commandLists[0]->Close();  // 초기 상태를 닫힌 상태로 맞춰 RenderFrame의 Reset() 호출과 짝을 맞춘다

    if (FAILED(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(m_fence.GetAddressOf()))))
    {
        return false;
    }
    m_fenceValue = 1;
    m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (m_fenceEvent == nullptr)
    {
        return false;
    }

    m_uiManager = std::make_unique<ImGuiManagerDX12>(windowHandle, m_device.Get(), m_commandQueue.Get(),
                                                       static_cast<int>(kBackBufferCount), DXGI_FORMAT_R8G8B8A8_UNORM);

    if (!CreateBaselinePipeline() || !CreateInstancingPipeline())
    {
        return false;
    }

    return true;
}

bool DirectX12Renderer::CreateBaselinePipeline()
{
    const std::vector<uint8_t> vsBytecode = ShaderBytecodeLoader::Load("shaders/directx12/Baseline.vs.cso");
    const std::vector<uint8_t> psBytecode = ShaderBytecodeLoader::Load("shaders/directx12/Baseline.ps.cso");

    D3D12_ROOT_PARAMETER rootParameter{};
    rootParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameter.Descriptor.ShaderRegister = 0;
    rootParameter.Descriptor.RegisterSpace = 0;
    rootParameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

    D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc{};
    rootSignatureDesc.NumParameters = 1;
    rootSignatureDesc.pParameters = &rootParameter;
    rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob;
    Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;
    if (FAILED(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1,
                                            signatureBlob.GetAddressOf(), errorBlob.GetAddressOf())))
    {
        return false;
    }
    if (FAILED(m_device->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(),
                                              IID_PPV_ARGS(m_rootSignature.GetAddressOf()))))
    {
        return false;
    }

    // NORMAL은 이번 사이클 셰이더가 실제로 읽지 않지만(라이팅 범위 밖), ModelMesh의 실제 정점
    // 레이아웃(position + normal)을 그대로 반영하기 위해 입력 슬롯 1로 선언해 둔다.
    static const D3D12_INPUT_ELEMENT_DESC kInputElements[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}};

    D3D12_RASTERIZER_DESC rasterizerDesc{};
    rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
    rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
    rasterizerDesc.DepthClipEnable = TRUE;

    D3D12_BLEND_DESC blendDesc{};
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

    D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
    depthStencilDesc.DepthEnable = FALSE;
    depthStencilDesc.StencilEnable = FALSE;

    D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineDesc{};
    pipelineDesc.InputLayout = {kInputElements, 2};
    pipelineDesc.pRootSignature = m_rootSignature.Get();
    pipelineDesc.VS = {vsBytecode.data(), vsBytecode.size()};
    pipelineDesc.PS = {psBytecode.data(), psBytecode.size()};
    pipelineDesc.RasterizerState = rasterizerDesc;
    pipelineDesc.BlendState = blendDesc;
    pipelineDesc.DepthStencilState = depthStencilDesc;
    pipelineDesc.SampleMask = UINT_MAX;
    pipelineDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    pipelineDesc.NumRenderTargets = 1;
    pipelineDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    pipelineDesc.SampleDesc.Count = 1;

    if (FAILED(m_device->CreateGraphicsPipelineState(&pipelineDesc, IID_PPV_ARGS(m_pipelineState.GetAddressOf()))))
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

    const UINT64 positionBytes = mesh.positions.size() * sizeof(Vec3);
    m_positionBuffer = CreateUploadBufferWithData(m_device.Get(), mesh.positions.data(), positionBytes);
    if (!m_positionBuffer)
    {
        return false;
    }
    m_positionBufferView.BufferLocation = m_positionBuffer->GetGPUVirtualAddress();
    m_positionBufferView.StrideInBytes = sizeof(float) * 3;
    m_positionBufferView.SizeInBytes = static_cast<UINT>(positionBytes);

    const UINT64 normalBytes = mesh.normals.size() * sizeof(Vec3);
    m_normalBuffer = CreateUploadBufferWithData(m_device.Get(), mesh.normals.data(), normalBytes);
    if (!m_normalBuffer)
    {
        return false;
    }
    m_normalBufferView.BufferLocation = m_normalBuffer->GetGPUVirtualAddress();
    m_normalBufferView.StrideInBytes = sizeof(float) * 3;
    m_normalBufferView.SizeInBytes = static_cast<UINT>(normalBytes);

    m_indexCount = static_cast<UINT>(mesh.indices.size());
    const UINT64 indexBytes = mesh.indices.size() * sizeof(uint32_t);
    m_indexBuffer = CreateUploadBufferWithData(m_device.Get(), mesh.indices.data(), indexBytes);
    if (!m_indexBuffer)
    {
        return false;
    }
    m_indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
    m_indexBufferView.Format = DXGI_FORMAT_R32_UINT;
    m_indexBufferView.SizeInBytes = static_cast<UINT>(indexBytes);

    // 루트 CBV는 디스크립터 뷰가 아니라 리소스 GPU 가상 주소를 직접 바인딩하므로 256바이트 정렬
    // 제약(CreateConstantBufferView 전용 규칙)이 적용되지 않는다 - Matrix4x4 크기(64바이트) 그대로 할당한다.
    m_constantBuffer = CreateUploadBuffer(m_device.Get(), sizeof(Matrix4x4));
    if (!m_constantBuffer)
    {
        return false;
    }
    const D3D12_RANGE noReadRange{0, 0};
    if (FAILED(m_constantBuffer->Map(0, &noReadRange, &m_constantBufferMappedData)))
    {
        return false;
    }

    return true;
}

bool DirectX12Renderer::CreateInstancingPipeline()
{
    const std::vector<uint8_t> vsBytecode = ShaderBytecodeLoader::Load("shaders/directx12/Instancing.vs.cso");
    const std::vector<uint8_t> psBytecode = ShaderBytecodeLoader::Load("shaders/directx12/Baseline.ps.cso");

    // INSTANCE_WORLD는 float4x4 하나를 TEXCOORD 스타일 4개의 float4 원소로 쪼개 표현하는 D3D의
    // 표준 관례다 - 각 원소는 입력 슬롯 1(인스턴스 버퍼)에서 인스턴스마다 한 번씩(InstanceDataStepRate=1) 읽힌다.
    // 루트 시그니처는 Baseline과 동일(단일 루트 CBV, b0)해서 재사용한다.
    static const D3D12_INPUT_ELEMENT_DESC kInstancingInputElements[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"INSTANCE_WORLD", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1},
        {"INSTANCE_WORLD", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 16, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1},
        {"INSTANCE_WORLD", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 32, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1},
        {"INSTANCE_WORLD", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 48, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1},
    };

    D3D12_RASTERIZER_DESC rasterizerDesc{};
    rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
    rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
    rasterizerDesc.DepthClipEnable = TRUE;

    D3D12_BLEND_DESC blendDesc{};
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

    D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
    depthStencilDesc.DepthEnable = FALSE;
    depthStencilDesc.StencilEnable = FALSE;

    D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineDesc{};
    pipelineDesc.InputLayout = {kInstancingInputElements, 5};
    pipelineDesc.pRootSignature = m_rootSignature.Get();
    pipelineDesc.VS = {vsBytecode.data(), vsBytecode.size()};
    pipelineDesc.PS = {psBytecode.data(), psBytecode.size()};
    pipelineDesc.RasterizerState = rasterizerDesc;
    pipelineDesc.BlendState = blendDesc;
    pipelineDesc.DepthStencilState = depthStencilDesc;
    pipelineDesc.SampleMask = UINT_MAX;
    pipelineDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    pipelineDesc.NumRenderTargets = 1;
    pipelineDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    pipelineDesc.SampleDesc.Count = 1;

    return SUCCEEDED(
        m_device->CreateGraphicsPipelineState(&pipelineDesc, IID_PPV_ARGS(m_instancingPipelineState.GetAddressOf())));
}

bool DirectX12Renderer::EnsureInstanceBufferCapacity(UINT instanceCount)
{
    if (m_instanceBuffer && instanceCount <= m_instanceBufferCapacity)
    {
        return true;
    }

    if (m_instanceBuffer && m_instanceBufferMappedData)
    {
        m_instanceBuffer->Unmap(0, nullptr);
        m_instanceBufferMappedData = nullptr;
    }

    m_instanceBuffer =
        CreateUploadBuffer(m_device.Get(), static_cast<UINT64>(instanceCount) * sizeof(Matrix4x4));
    if (!m_instanceBuffer)
    {
        return false;
    }

    const D3D12_RANGE noReadRange{0, 0};
    if (FAILED(m_instanceBuffer->Map(0, &noReadRange, &m_instanceBufferMappedData)))
    {
        return false;
    }
    m_instanceBufferCapacity = instanceCount;
    return true;
}

void DirectX12Renderer::RenderFrame(const InstanceSnapshot& snapshot)
{
    if (!m_commandQueue || !m_swapChain || !m_uiManager)
    {
        return;
    }

    m_uiManager->NewFrame();

    if (m_uiElementRegistry)
    {
        m_uiElementRegistry->RenderAll();
    }

    auto& commandAllocator = m_commandAllocators[0];
    auto& commandList = m_commandLists[0];

    commandAllocator->Reset();
    commandList->Reset(commandAllocator.Get(), nullptr);

    D3D12_RESOURCE_BARRIER toRenderTarget{};
    toRenderTarget.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    toRenderTarget.Transition.pResource = m_renderTargets[m_frameIndex].Get();
    toRenderTarget.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    toRenderTarget.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    toRenderTarget.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    commandList->ResourceBarrier(1, &toRenderTarget);

    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
    rtvHandle.ptr += static_cast<SIZE_T>(m_frameIndex) * m_rtvDescriptorSize;

    constexpr float kClearColor[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    commandList->ClearRenderTargetView(rtvHandle, kClearColor, 0, nullptr);

    commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

    const D3D12_VIEWPORT viewport{0.0f, 0.0f, static_cast<float>(m_width), static_cast<float>(m_height), 0.0f, 1.0f};
    const D3D12_RECT scissorRect{0, 0, static_cast<LONG>(m_width), static_cast<LONG>(m_height)};

    const bool canDrawInstanced = !snapshot.worldMatrices.empty() && m_instancingPipelineState && m_rootSignature &&
                                   m_positionBuffer && m_indexBuffer && m_constantBufferMappedData;
    if (canDrawInstanced)
    {
        const UINT instanceCount = static_cast<UINT>(snapshot.worldMatrices.size());
        if (EnsureInstanceBufferCapacity(instanceCount))
        {
            std::memcpy(m_instanceBufferMappedData, snapshot.worldMatrices.data(), instanceCount * sizeof(Matrix4x4));

            const Matrix4x4 viewProj = Matrix4x4::Identity();
            std::memcpy(m_constantBufferMappedData, &viewProj, sizeof(Matrix4x4));

            commandList->RSSetViewports(1, &viewport);
            commandList->RSSetScissorRects(1, &scissorRect);

            D3D12_VERTEX_BUFFER_VIEW instanceBufferView{};
            instanceBufferView.BufferLocation = m_instanceBuffer->GetGPUVirtualAddress();
            instanceBufferView.StrideInBytes = sizeof(Matrix4x4);
            instanceBufferView.SizeInBytes = instanceCount * sizeof(Matrix4x4);
            const D3D12_VERTEX_BUFFER_VIEW vertexBufferViews[] = {m_positionBufferView, instanceBufferView};

            commandList->SetGraphicsRootSignature(m_rootSignature.Get());
            commandList->SetPipelineState(m_instancingPipelineState.Get());
            commandList->SetGraphicsRootConstantBufferView(0, m_constantBuffer->GetGPUVirtualAddress());
            commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            commandList->IASetVertexBuffers(0, 2, vertexBufferViews);
            commandList->IASetIndexBuffer(&m_indexBufferView);
            commandList->DrawIndexedInstanced(m_indexCount, instanceCount, 0, 0, 0);
        }
    }
    else if (m_pipelineState && m_rootSignature && m_constantBufferMappedData)
    {
        const Matrix4x4 worldViewProj = Matrix4x4::Identity();
        std::memcpy(m_constantBufferMappedData, &worldViewProj, sizeof(Matrix4x4));

        commandList->RSSetViewports(1, &viewport);
        commandList->RSSetScissorRects(1, &scissorRect);

        const D3D12_VERTEX_BUFFER_VIEW vertexBufferViews[] = {m_positionBufferView, m_normalBufferView};

        commandList->SetGraphicsRootSignature(m_rootSignature.Get());
        commandList->SetPipelineState(m_pipelineState.Get());
        commandList->SetGraphicsRootConstantBufferView(0, m_constantBuffer->GetGPUVirtualAddress());
        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        commandList->IASetVertexBuffers(0, 2, vertexBufferViews);
        commandList->IASetIndexBuffer(&m_indexBufferView);
        commandList->DrawIndexedInstanced(m_indexCount, 1, 0, 0, 0);
    }

    m_uiManager->RenderWithCommandList(commandList.Get());

    D3D12_RESOURCE_BARRIER toPresent{};
    toPresent.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    toPresent.Transition.pResource = m_renderTargets[m_frameIndex].Get();
    toPresent.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    toPresent.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    toPresent.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    commandList->ResourceBarrier(1, &toPresent);

    commandList->Close();

    ID3D12CommandList* commandListsToExecute[] = {commandList.Get()};
    m_commandQueue->ExecuteCommandLists(1, commandListsToExecute);

    m_swapChain->Present(1, 0);

    WaitForGpu();
    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
}

bool DirectX12Renderer::DebugReadBackInstanceBuffer(std::vector<Matrix4x4>& outMatrices) const
{
    if (!m_instanceBufferMappedData || m_instanceBufferCapacity == 0)
    {
        return false;
    }

    outMatrices.resize(m_instanceBufferCapacity);
    std::memcpy(outMatrices.data(), m_instanceBufferMappedData, m_instanceBufferCapacity * sizeof(Matrix4x4));
    return true;
}

void DirectX12Renderer::OnResize(int width, int height)
{
    if (!m_swapChain || width <= 0 || height <= 0)
    {
        return;
    }

    m_width = static_cast<UINT>(width);
    m_height = static_cast<UINT>(height);

    WaitForGpu();
    ReleaseRenderTargets();

    const HRESULT hr = m_swapChain->ResizeBuffers(kBackBufferCount, static_cast<UINT>(width),
                                                    static_cast<UINT>(height), DXGI_FORMAT_UNKNOWN, 0);
    if (SUCCEEDED(hr))
    {
        m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
        CreateRenderTargets();
    }
}

void DirectX12Renderer::Shutdown()
{
    WaitForGpu();

    if (m_uiManager)
    {
        m_uiManager->Shutdown();
        m_uiManager.reset();
    }

    if (m_fenceEvent != nullptr)
    {
        CloseHandle(m_fenceEvent);
        m_fenceEvent = nullptr;
    }

    if (m_constantBuffer && m_constantBufferMappedData)
    {
        m_constantBuffer->Unmap(0, nullptr);
        m_constantBufferMappedData = nullptr;
    }
    if (m_instanceBuffer && m_instanceBufferMappedData)
    {
        m_instanceBuffer->Unmap(0, nullptr);
        m_instanceBufferMappedData = nullptr;
    }
    m_instanceBuffer.Reset();
    m_instancingPipelineState.Reset();
    m_constantBuffer.Reset();
    m_indexBuffer.Reset();
    m_normalBuffer.Reset();
    m_positionBuffer.Reset();
    m_pipelineState.Reset();
    m_rootSignature.Reset();

    ReleaseRenderTargets();
    m_rtvHeap.Reset();
    for (auto& commandList : m_commandLists)
    {
        commandList.Reset();
    }
    for (auto& commandAllocator : m_commandAllocators)
    {
        commandAllocator.Reset();
    }
    m_fence.Reset();
    m_swapChain.Reset();
    m_commandQueue.Reset();
    m_device.Reset();
}

bool DirectX12Renderer::HandleUiMessage(HWND windowHandle, UINT message, WPARAM wParam, LPARAM lParam)
{
    return m_uiManager && m_uiManager->HandleWin32Message(windowHandle, message, wParam, lParam);
}

void DirectX12Renderer::SetUiElementRegistry(IUiElementRegistry& registry)
{
    m_uiElementRegistry = &registry;
}
