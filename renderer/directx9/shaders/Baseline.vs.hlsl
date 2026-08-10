// Author: Claude
// Description: DX9(Shader Model 3.0) Baseline Vertex Shader. WorldViewProj 상수 하나로 정점을
//              변환한다 - 이번 사이클은 카메라/투영 시스템이 범위 밖이라 상수는 Identity 또는
//              InstanceSnapshot의 월드 행렬을 그대로 받는다(호출부인 DirectX9Renderer 주석 참고).
// Input: c0~c3 - WorldViewProj(row_major float4x4), POSITION(정점 위치)
// Output: POSITION(변환된 클립 공간 좌표)
// Notes: DX9는 cbuffer 리소스가 없어 개별 상수 레지스터(c0~c3)로 4x4 행렬을 받는다 - CPU 쪽
//        Matrix4x4는 row-major 저장이라 row_major 한정자로 선언해 별도 Transpose 없이 그대로 올린다.
// Date: 2026-07-28
row_major float4x4 WorldViewProj : register(c0);

struct VS_INPUT
{
    float3 Position : POSITION;
};

struct VS_OUTPUT
{
    float4 Position : POSITION;
};

VS_OUTPUT VSMain(VS_INPUT input)
{
    VS_OUTPUT output;
    output.Position = mul(float4(input.Position, 1.0f), WorldViewProj);
    return output;
}
