#pragma once

#include <wrl/client.h>

#include <vector>

#include <d3d12.h>
#include <dxgi1_4.h>

// Author: Claude
// Description: DX12 Compute Shader 최소 데모 - 렌더 루프(IRenderer/DirectX12Renderer)와 완전히
//              독립된 헤드리스 디바이스/커맨드큐를 스스로 생성해 RWStructuredBuffer 변환(값 2배)을
//              Dispatch하고 CPU로 리드백한다. 실제 렌더링 파이프라인에는 연결하지 않는다
//              (docs/strategy/DirectX_Shader_기초연결_20260726_1400.md 체크리스트 9 참고).
// Input: Initialize - (해당 없음) / TransformData - 입력 float 배열
// Output: Initialize - 성공 여부 / TransformData - 각 원소가 2배가 된 결과(실패 시 빈 벡터)
// Notes: 스왑체인/윈도우가 필요 없는 순수 계산 작업이라 하드웨어 실패 시 WARP 어댑터로 폴백한다
//        (DirectX12Renderer와 동일한 실사용 목적 폴백 패턴). UPLOAD 힙(초기값 업로드) ->
//        DEFAULT 힙(UAV, 실제 연산 대상) -> READBACK 힙(결과 회수) 3단 버퍼 구성을 매
//        TransformData() 호출마다 새로 만든다 - 데모용 1회성 연산이라 재사용 캐싱은 하지 않는다.
// Date: 2026-07-28
class DirectX12ComputeDemo
{
public:
    ~DirectX12ComputeDemo();

    bool Initialize();
    std::vector<float> TransformData(const std::vector<float>& input);

private:
    Microsoft::WRL::ComPtr<ID3D12Device> m_device;
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_commandQueue;
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_commandAllocator;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_commandList;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pipelineState;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_uavHeap;
    Microsoft::WRL::ComPtr<ID3D12Fence> m_fence;
    HANDLE m_fenceEvent = nullptr;
    UINT64 m_fenceValue = 0;

    bool CreateDevice();
    void WaitForGpu();
};
