// Author: Claude
// Description: DX12 Baseline Vertex Shader. WorldViewProj 상수 하나로 정점을 변환한다 - 이번
//              사이클은 카메라/투영 시스템이 범위 밖이라 상수는 Identity 또는 InstanceSnapshot의
//              월드 행렬을 그대로 받는다(호출부인 DirectX12Renderer 주석 참고).
// Input: b0 - WorldViewProjBuffer(row_major float4x4), POSITION(정점 위치)
// Output: SV_POSITION(변환된 클립 공간 좌표)
// Notes: CPU 쪽 Matrix4x4는 row-major 저장이라 cbuffer 필드를 row_major로 선언해 별도 Transpose
//        없이 그대로 올린다.
// Date: 2026-07-28
cbuffer WorldViewProjBuffer : register(b0)
{
    row_major float4x4 WorldViewProj;
};

struct VSInput
{
    float3 Position : POSITION;
};

struct VSOutput
{
    float4 Position : SV_POSITION;
};

VSOutput VSMain(VSInput input)
{
    VSOutput output;
    output.Position = mul(float4(input.Position, 1.0f), WorldViewProj);
    return output;
}
