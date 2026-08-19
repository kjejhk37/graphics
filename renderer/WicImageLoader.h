#pragma once

#include <cstdint>
#include <string>
#include <vector>

// Author: Claude
// Description: WIC(Windows Imaging Component)를 감싸는 내부 wrapper. 파일 경로를 받아 RGBA8(픽셀당 4바이트,
//              top-down) 픽셀 버퍼로 디코딩한다. PNG/JPEG/BMP 등 WIC가 지원하는 포맷을 그대로 지원한다.
// Input: (해당 없음 - Load()의 Input 참고)
// Output: (해당 없음)
// Notes: <wincodec.h>/COM을 아는 유일한 파일이다 - 다른 모듈은 이 클래스를 통해서만 이미지를 디코딩한다.
//        Windows SDK에 포함된 시스템 컴포넌트(windowscodecs.lib)라 FetchContent로 받는 서드파티
//        라이브러리가 아니다 - dependency_eval.md 대상이 아니라고 판단한 이유(Brainstorming 참고).
// Date: 2026-08-19
class WicImageLoader
{
public:
    struct DecodedImage
    {
        std::vector<uint8_t> pixels;  // width * height * 4바이트, RGBA8, top-down, 행 간 패딩 없음
        uint32_t width = 0;
        uint32_t height = 0;
    };

    // Author: Claude
    // Description: 이미지 파일을 RGBA8 픽셀 버퍼로 디코딩한다.
    // Input: filePath - 로드할 이미지 파일 경로(UTF-8)
    // Output: outImage - 성공 시 디코딩된 픽셀/치수. 반환값 - 성공 여부.
    // Notes: 파일이 없거나 WIC가 지원하지 않는 포맷이면 false. 원본이 RGBA8이 아니어도(예: 24bpp RGB,
    //        인덱스 팔레트 등) WIC의 포맷 변환기가 내부적으로 RGBA8로 맞춰준다.
    // Date: 2026-08-19
    static bool Load(const std::string& filePath, DecodedImage& outImage);
};
