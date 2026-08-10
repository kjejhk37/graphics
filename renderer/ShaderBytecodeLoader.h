#pragma once

#include <cstdint>
#include <string>
#include <vector>

// Author: Claude
// Description: 오프라인 컴파일된 셰이더 바이트코드(.cso)를 파일에서 읽어오는 백엔드 공용 유틸리티.
//              DX9/11/12 렌더러가 CreateVertexShader/CreatePixelShader/CreateComputeShader 등에
//              넘길 바이트코드 blob을 전부 이 함수를 통해 얻는다 - "블롭을 어디서 받아오는지"만
//              캡슐화하며, 오프라인/런타임 컴파일 여부가 바뀌어도 호출부는 이 함수만 안다.
// Input: Load - .cso 파일 경로
// Output: Load - 파일 내용을 담은 바이트 벡터(비어있지 않음이 보장됨)
// Notes: 파일이 없거나 읽기에 실패하거나 내용이 비어있으면 std::runtime_error를 던진다 - 셰이더 로드
//        실패는 렌더러 초기화를 계속할 수 없는 치명적 상황이라 옵셔널 대신 예외로 표현한다.
// Date: 2026-07-28
namespace ShaderBytecodeLoader
{
    std::vector<uint8_t> Load(const std::string& csoPath);
}
