// Author: Claude
// Description: DX12 Compute Shader 최소 데모. RWStructuredBuffer의 각 원소를 2배로 만든다 -
//              렌더 루프와 무관한 독립 검증용(DirectX12ComputeDemo 참고).
// Input: u0 - Data(RWStructuredBuffer<float>)
// Output: (해당 없음 - Data를 제자리에서(in-place) 수정)
// Notes: 디스패치된 스레드 수가 실제 원소 개수보다 많아도 안전하다 - Direct3D는 UAV 구조화 버퍼의
//        선언된 범위(NumElements) 밖 접근에 대해 크래시 없음을 보장한다(쓰기는 무시됨) - 그래서
//        별도 경계 검사(bounds check) 상수 없이 최소 형태로 유지한다.
// Date: 2026-07-28
RWStructuredBuffer<float> Data : register(u0);

[numthreads(64, 1, 1)]
void CSMain(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    Data[dispatchThreadId.x] *= 2.0f;
}
