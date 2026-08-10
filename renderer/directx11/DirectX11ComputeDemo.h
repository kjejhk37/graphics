#pragma once

#include <wrl/client.h>

#include <vector>

#include <d3d11.h>

// Author: Claude
// Description: DX11 Compute Shader 최소 데모 - 렌더 루프(IRenderer/DirectX11Renderer)와 완전히
//              독립된 헤드리스 디바이스를 스스로 생성해 RWStructuredBuffer 변환(값 2배)을
//              Dispatch하고 CPU로 리드백한다. 실제 렌더링 파이프라인에는 연결하지 않는다
//              (docs/strategy/DirectX_Shader_기초연결_20260726_1400.md 체크리스트 9 참고).
// Input: Initialize - (해당 없음) / TransformData - 입력 float 배열
// Output: Initialize - 성공 여부 / TransformData - 각 원소가 2배가 된 결과(실패 시 빈 벡터)
// Notes: 스왑체인/윈도우가 필요 없는 순수 계산 작업이라 하드웨어 실패 시 WARP로 폴백한다
//        (DirectX11Renderer와 동일한 실사용 목적 폴백 패턴).
// Date: 2026-07-28
class DirectX11ComputeDemo
{
public:
    bool Initialize();
    std::vector<float> TransformData(const std::vector<float>& input);

private:
    Microsoft::WRL::ComPtr<ID3D11Device> m_device;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_context;
    Microsoft::WRL::ComPtr<ID3D11ComputeShader> m_computeShader;
};
