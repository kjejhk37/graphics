#include "graphics/renderer/directx12/DirectX12ComputeDemo.h"

#include <cstring>

#include "graphics/renderer/ShaderBytecodeLoader.h"

namespace
{
    Microsoft::WRL::ComPtr<ID3D12Resource> CreateBuffer(ID3D12Device* device, D3D12_HEAP_TYPE heapType,
                                                          UINT64 sizeInBytes, D3D12_RESOURCE_FLAGS flags,
                                                          D3D12_RESOURCE_STATES initialState)
    {
        D3D12_HEAP_PROPERTIES heapProperties{};
        heapProperties.Type = heapType;

        D3D12_RESOURCE_DESC resourceDesc{};
        resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        resourceDesc.Width = sizeInBytes;
        resourceDesc.Height = 1;
        resourceDesc.DepthOrArraySize = 1;
        resourceDesc.MipLevels = 1;
        resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
        resourceDesc.SampleDesc.Count = 1;
        resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        resourceDesc.Flags = flags;

        Microsoft::WRL::ComPtr<ID3D12Resource> resource;
        device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc, initialState, nullptr,
                                         IID_PPV_ARGS(&resource));
        return resource;
    }
}

DirectX12ComputeDemo::~DirectX12ComputeDemo()
{
    if (m_fenceEvent != nullptr)
    {
        CloseHandle(m_fenceEvent);
        m_fenceEvent = nullptr;
    }
}

bool DirectX12ComputeDemo::CreateDevice()
{
    if (SUCCEEDED(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(m_device.GetAddressOf()))))
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

bool DirectX12ComputeDemo::Initialize()
{
    if (!CreateDevice())
    {
        return false;
    }

    D3D12_COMMAND_QUEUE_DESC queueDesc{};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    if (FAILED(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(m_commandQueue.GetAddressOf()))))
    {
        return false;
    }

    if (FAILED(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
                                                 IID_PPV_ARGS(m_commandAllocator.GetAddressOf()))))
    {
        return false;
    }
    if (FAILED(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator.Get(), nullptr,
                                            IID_PPV_ARGS(m_commandList.GetAddressOf()))))
    {
        return false;
    }
    m_commandList->Close();

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

    D3D12_DESCRIPTOR_RANGE uavRange{};
    uavRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
    uavRange.NumDescriptors = 1;
    uavRange.BaseShaderRegister = 0;

    D3D12_ROOT_PARAMETER rootParameter{};
    rootParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParameter.DescriptorTable.NumDescriptorRanges = 1;
    rootParameter.DescriptorTable.pDescriptorRanges = &uavRange;
    rootParameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc{};
    rootSignatureDesc.NumParameters = 1;
    rootSignatureDesc.pParameters = &rootParameter;

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

    const std::vector<uint8_t> csBytecode = ShaderBytecodeLoader::Load("shaders/directx12/Transform.cs.cso");
    D3D12_COMPUTE_PIPELINE_STATE_DESC pipelineDesc{};
    pipelineDesc.pRootSignature = m_rootSignature.Get();
    pipelineDesc.CS = {csBytecode.data(), csBytecode.size()};
    if (FAILED(m_device->CreateComputePipelineState(&pipelineDesc, IID_PPV_ARGS(m_pipelineState.GetAddressOf()))))
    {
        return false;
    }

    D3D12_DESCRIPTOR_HEAP_DESC uavHeapDesc{};
    uavHeapDesc.NumDescriptors = 1;
    uavHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    uavHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    return SUCCEEDED(m_device->CreateDescriptorHeap(&uavHeapDesc, IID_PPV_ARGS(m_uavHeap.GetAddressOf())));
}

std::vector<float> DirectX12ComputeDemo::TransformData(const std::vector<float>& input)
{
    if (!m_device || input.empty())
    {
        return {};
    }

    const UINT64 byteWidth = input.size() * sizeof(float);

    Microsoft::WRL::ComPtr<ID3D12Resource> uploadBuffer =
        CreateBuffer(m_device.Get(), D3D12_HEAP_TYPE_UPLOAD, byteWidth, D3D12_RESOURCE_FLAG_NONE,
                     D3D12_RESOURCE_STATE_GENERIC_READ);
    Microsoft::WRL::ComPtr<ID3D12Resource> dataBuffer =
        CreateBuffer(m_device.Get(), D3D12_HEAP_TYPE_DEFAULT, byteWidth, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
                     D3D12_RESOURCE_STATE_COPY_DEST);
    Microsoft::WRL::ComPtr<ID3D12Resource> readbackBuffer = CreateBuffer(
        m_device.Get(), D3D12_HEAP_TYPE_READBACK, byteWidth, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_COPY_DEST);
    if (!uploadBuffer || !dataBuffer || !readbackBuffer)
    {
        return {};
    }

    void* uploadMappedData = nullptr;
    const D3D12_RANGE noReadRange{0, 0};
    if (FAILED(uploadBuffer->Map(0, &noReadRange, &uploadMappedData)))
    {
        return {};
    }
    std::memcpy(uploadMappedData, input.data(), byteWidth);
    uploadBuffer->Unmap(0, nullptr);

    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
    uavDesc.Format = DXGI_FORMAT_UNKNOWN;
    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
    uavDesc.Buffer.NumElements = static_cast<UINT>(input.size());
    uavDesc.Buffer.StructureByteStride = sizeof(float);
    m_device->CreateUnorderedAccessView(dataBuffer.Get(), nullptr, &uavDesc,
                                         m_uavHeap->GetCPUDescriptorHandleForHeapStart());

    m_commandAllocator->Reset();
    m_commandList->Reset(m_commandAllocator.Get(), m_pipelineState.Get());

    m_commandList->CopyResource(dataBuffer.Get(), uploadBuffer.Get());

    D3D12_RESOURCE_BARRIER toUav{};
    toUav.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    toUav.Transition.pResource = dataBuffer.Get();
    toUav.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    toUav.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    toUav.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    m_commandList->ResourceBarrier(1, &toUav);

    ID3D12DescriptorHeap* heaps[] = {m_uavHeap.Get()};
    m_commandList->SetDescriptorHeaps(1, heaps);
    m_commandList->SetComputeRootSignature(m_rootSignature.Get());
    m_commandList->SetComputeRootDescriptorTable(0, m_uavHeap->GetGPUDescriptorHandleForHeapStart());

    const UINT threadGroupCount = (static_cast<UINT>(input.size()) + 63) / 64;
    m_commandList->Dispatch(threadGroupCount, 1, 1);

    D3D12_RESOURCE_BARRIER toCopySource{};
    toCopySource.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    toCopySource.Transition.pResource = dataBuffer.Get();
    toCopySource.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    toCopySource.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
    toCopySource.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    m_commandList->ResourceBarrier(1, &toCopySource);

    m_commandList->CopyResource(readbackBuffer.Get(), dataBuffer.Get());

    m_commandList->Close();
    ID3D12CommandList* commandListsToExecute[] = {m_commandList.Get()};
    m_commandQueue->ExecuteCommandLists(1, commandListsToExecute);
    WaitForGpu();

    void* readbackMappedData = nullptr;
    if (FAILED(readbackBuffer->Map(0, nullptr, &readbackMappedData)))
    {
        return {};
    }
    std::vector<float> result(input.size());
    std::memcpy(result.data(), readbackMappedData, byteWidth);
    const D3D12_RANGE emptyWriteRange{0, 0};
    readbackBuffer->Unmap(0, &emptyWriteRange);

    return result;
}

void DirectX12ComputeDemo::WaitForGpu()
{
    const UINT64 fenceValueToSignal = m_fenceValue;
    m_commandQueue->Signal(m_fence.Get(), fenceValueToSignal);
    ++m_fenceValue;

    if (m_fence->GetCompletedValue() < fenceValueToSignal)
    {
        m_fence->SetEventOnCompletion(fenceValueToSignal, m_fenceEvent);
        WaitForSingleObject(m_fenceEvent, INFINITE);
    }
}
