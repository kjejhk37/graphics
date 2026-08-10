// Author: Claude
// Description: DX9(Shader Model 3.0) 하드웨어 인스턴싱 Vertex Shader. 정점(POSITION)은 스트림 0,
//              인스턴스 월드 행렬은 스트림 1에서 TEXCOORD1~4(float4 4개)로 나뉘어 들어온다 -
//              DX9는 cbuffer/row_major 자동 패킹이 아니라 스트림 주파수(SetStreamSourceFreq)로
//              인스턴싱을 흉내내므로, 행렬을 4개의 float4로 수동 조립한다.
// Input: 스트림 0 - POSITION(정점 위치) / 스트림 1(인스턴스별) - TEXCOORD1~4(월드 행렬의 각 행)
// Output: POSITION(변환된 클립 공간 좌표)
// Notes: 카메라/투영 시스템은 범위 밖이라 인스턴스 월드 행렬을 그대로 클립 공간 좌표로 사용한다
//        (DirectX9Renderer 참고). float4x4(row0,row1,row2,row3) 생성자는 각 인자를 행(row)으로
//        조립하므로, CPU의 row-major Matrix4x4를 그대로 각 TEXCOORD에 올리면 별도 Transpose가
//        필요 없다.
// Date: 2026-07-28
struct VS_INPUT
{
    float3 Position : POSITION;
    float4 InstanceWorldRow0 : TEXCOORD1;
    float4 InstanceWorldRow1 : TEXCOORD2;
    float4 InstanceWorldRow2 : TEXCOORD3;
    float4 InstanceWorldRow3 : TEXCOORD4;
};

struct VS_OUTPUT
{
    float4 Position : POSITION;
};

VS_OUTPUT VSMain(VS_INPUT input)
{
    const float4x4 instanceWorld =
        float4x4(input.InstanceWorldRow0, input.InstanceWorldRow1, input.InstanceWorldRow2, input.InstanceWorldRow3);

    VS_OUTPUT output;
    output.Position = mul(float4(input.Position, 1.0f), instanceWorld);
    return output;
}
