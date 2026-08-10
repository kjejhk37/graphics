// Author: Claude
// Description: DX9(Shader Model 3.0) Baseline Pixel Shader. 라이팅/머티리얼은 이번 사이클 범위 밖이라
//              보간 입력 없이 고정 단색만 출력한다(파이프라인 스모크 테스트 목적).
// Input: (해당 없음)
// Output: COLOR - 고정 단색(주황색)
// Notes: 없음.
// Date: 2026-07-28
float4 PSMain() : COLOR
{
    return float4(1.0f, 0.5f, 0.0f, 1.0f);
}
