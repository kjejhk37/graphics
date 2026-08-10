// Author: Claude
// Description: DX11 Instancing Vertex Shader. 정점(POSITION)은 입력 슬롯 0에서 매 정점마다,
//              인스턴스 월드 행렬(INSTANCE_WORLD)은 입력 슬롯 1에서 인스턴스마다 한 번씩 읽는다.
// Input: 슬롯 0 - POSITION(정점 위치) / 슬롯 1(인스턴스별) - INSTANCE_WORLD(row_major float4x4,
//        4개의 TEXCOORD 스타일 float4 원소로 분해되어 전달됨 - D3D11_INPUT_PER_INSTANCE_DATA)
// Output: SV_POSITION(변환된 클립 공간 좌표)
// Notes: 카메라/투영 시스템은 범위 밖이라 ViewProj는 Identity를 사용한다(DirectX11Renderer 참고).
//        CPU 쪽 Matrix4x4는 row-major 저장이라 row_major로 선언해 별도 Transpose 없이 그대로 올린다.
// Date: 2026-07-28
cbuffer ViewProjBuffer : register(b0)
{
    row_major float4x4 ViewProj;
};

struct VSInput
{
    float3 Position : POSITION;
    row_major float4x4 InstanceWorld : INSTANCE_WORLD;
};

struct VSOutput
{
    float4 Position : SV_POSITION;
};

VSOutput VSMain(VSInput input)
{
    VSOutput output;
    const float4 worldPosition = mul(float4(input.Position, 1.0f), input.InstanceWorld);
    output.Position = mul(worldPosition, ViewProj);
    return output;
}
