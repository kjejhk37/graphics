#include "graphics/renderer/directx11/DirectX11ComputeDemo.h"

#include <cstring>

#include "graphics/renderer/ShaderBytecodeLoader.h"

bool DirectX11ComputeDemo::Initialize()
{
    HRESULT hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION,
                                    m_device.GetAddressOf(), nullptr, m_context.GetAddressOf());
    if (FAILED(hr))
    {
        hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION,
                                m_device.GetAddressOf(), nullptr, m_context.GetAddressOf());
    }
    if (FAILED(hr))
    {
        return false;
    }

    const std::vector<uint8_t> csBytecode = ShaderBytecodeLoader::Load("shaders/directx11/Transform.cs.cso");
    return SUCCEEDED(
        m_device->CreateComputeShader(csBytecode.data(), csBytecode.size(), nullptr, m_computeShader.GetAddressOf()));
}

std::vector<float> DirectX11ComputeDemo::TransformData(const std::vector<float>& input)
{
    if (!m_device || input.empty())
    {
        return {};
    }

    const UINT byteWidth = static_cast<UINT>(input.size() * sizeof(float));

    D3D11_BUFFER_DESC bufferDesc{};
    bufferDesc.ByteWidth = byteWidth;
    bufferDesc.Usage = D3D11_USAGE_DEFAULT;
    bufferDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
    bufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    bufferDesc.StructureByteStride = sizeof(float);

    D3D11_SUBRESOURCE_DATA initData{};
    initData.pSysMem = input.data();

    Microsoft::WRL::ComPtr<ID3D11Buffer> dataBuffer;
    if (FAILED(m_device->CreateBuffer(&bufferDesc, &initData, dataBuffer.GetAddressOf())))
    {
        return {};
    }

    D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
    uavDesc.Format = DXGI_FORMAT_UNKNOWN;
    uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
    uavDesc.Buffer.NumElements = static_cast<UINT>(input.size());

    Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> uav;
    if (FAILED(m_device->CreateUnorderedAccessView(dataBuffer.Get(), &uavDesc, uav.GetAddressOf())))
    {
        return {};
    }

    m_context->CSSetShader(m_computeShader.Get(), nullptr, 0);
    m_context->CSSetUnorderedAccessViews(0, 1, uav.GetAddressOf(), nullptr);
    const UINT threadGroupCount = (static_cast<UINT>(input.size()) + 63) / 64;
    m_context->Dispatch(threadGroupCount, 1, 1);

    ID3D11UnorderedAccessView* nullUav[] = {nullptr};
    m_context->CSSetUnorderedAccessViews(0, 1, nullUav, nullptr);

    D3D11_BUFFER_DESC stagingDesc = bufferDesc;
    stagingDesc.Usage = D3D11_USAGE_STAGING;
    stagingDesc.BindFlags = 0;
    stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

    Microsoft::WRL::ComPtr<ID3D11Buffer> stagingBuffer;
    if (FAILED(m_device->CreateBuffer(&stagingDesc, nullptr, stagingBuffer.GetAddressOf())))
    {
        return {};
    }
    m_context->CopyResource(stagingBuffer.Get(), dataBuffer.Get());

    D3D11_MAPPED_SUBRESOURCE mapped{};
    if (FAILED(m_context->Map(stagingBuffer.Get(), 0, D3D11_MAP_READ, 0, &mapped)))
    {
        return {};
    }
    std::vector<float> result(input.size());
    std::memcpy(result.data(), mapped.pData, byteWidth);
    m_context->Unmap(stagingBuffer.Get(), 0);

    return result;
}
