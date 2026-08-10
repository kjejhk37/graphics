#pragma once

#include <vector>

#include "platform/math/Matrix4x4.h"

// Author: Claude
// Description: Engine(producer)이 한 프레임 분량으로 계산한 인스턴스별 월드 행렬 묶음. Render(consumer)가
//              DoubleBufferPublisher<InstanceSnapshot>을 통해 읽어가는 페이로드 타입이다.
// Input: (해당 없음 - 데이터 구조체)
// Output: (해당 없음 - 데이터 구조체)
// Notes: 인스턴스 개수는 고정이 아니다 - InstanceUpdateWorker가 매 프레임 worldMatrices.resize()로
//        크기를 맞춘 뒤 채운다. 이 타입 자체는 스레드 안전성을 보장하지 않는다 - 안전성은
//        IFrameDataPublisher<InstanceSnapshot>(DoubleBufferPublisher)의 슬롯 분리가 책임진다.
// Date: 2026-07-28
struct InstanceSnapshot
{
    std::vector<Matrix4x4> worldMatrices;
};
