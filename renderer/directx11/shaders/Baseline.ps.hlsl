// Author: Claude
// Description: DX11 Baseline Pixel Shader. 라이팅/머티리얼은 이번 사이클 범위 밖이라 고정 단색만
//              출력한다(파이프라인 스모크 테스트 목적).
// Input: SV_POSITION(사용하지 않음)
// Output: SV_TARGET - 고정 단색(주황색)
// Notes: 없음.
// Date: 2026-07-28
struct PSInput
{
    float4 Position : SV_POSITION;
};

float4 PSMain(PSInput input) : SV_TARGET
{
    return float4(1.0f, 0.5f, 0.0f, 1.0f);
}
