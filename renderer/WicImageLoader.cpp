#include "graphics/renderer/WicImageLoader.h"

#include <wrl/client.h>

#include <windows.h>
#include <wincodec.h>

namespace
{
    // WIC의 CreateDecoderFromFilename은 와이드 문자열 경로를 요구한다 - 이 프로젝트는 소스를 UTF-8로
    // 고정하므로(CMakeLists.txt의 /utf-8 참고) filePath도 UTF-8이라고 가정하고 변환한다.
    std::wstring Utf8ToWide(const std::string& utf8)
    {
        if (utf8.empty())
        {
            return std::wstring();
        }
        const int requiredLength = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
        if (requiredLength <= 0)
        {
            return std::wstring();
        }
        std::wstring wide(static_cast<size_t>(requiredLength) - 1, L'\0');
        MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, wide.data(), requiredLength);
        return wide;
    }
}

bool WicImageLoader::Load(const std::string& filePath, DecodedImage& outImage)
{
    const HRESULT comInitResult = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    const bool comInitializedHere = SUCCEEDED(comInitResult);

    bool succeeded = false;
    do
    {
        Microsoft::WRL::ComPtr<IWICImagingFactory> factory;
        if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                                     IID_PPV_ARGS(factory.GetAddressOf()))))
        {
            break;
        }

        const std::wstring widePath = Utf8ToWide(filePath);
        Microsoft::WRL::ComPtr<IWICBitmapDecoder> decoder;
        if (FAILED(factory->CreateDecoderFromFilename(widePath.c_str(), nullptr, GENERIC_READ,
                                                        WICDecodeMetadataCacheOnDemand, decoder.GetAddressOf())))
        {
            break;
        }

        Microsoft::WRL::ComPtr<IWICBitmapFrameDecode> frame;
        if (FAILED(decoder->GetFrame(0, frame.GetAddressOf())))
        {
            break;
        }

        Microsoft::WRL::ComPtr<IWICFormatConverter> converter;
        if (FAILED(factory->CreateFormatConverter(converter.GetAddressOf())))
        {
            break;
        }
        if (FAILED(converter->Initialize(frame.Get(), GUID_WICPixelFormat32bppRGBA, WICBitmapDitherTypeNone, nullptr,
                                          0.0, WICBitmapPaletteTypeCustom)))
        {
            break;
        }

        UINT width = 0;
        UINT height = 0;
        if (FAILED(converter->GetSize(&width, &height)) || width == 0 || height == 0)
        {
            break;
        }

        std::vector<uint8_t> pixels(static_cast<size_t>(width) * height * 4);
        const UINT stride = width * 4;
        if (FAILED(converter->CopyPixels(nullptr, stride, static_cast<UINT>(pixels.size()), pixels.data())))
        {
            break;
        }

        outImage.pixels = std::move(pixels);
        outImage.width = width;
        outImage.height = height;
        succeeded = true;
    } while (false);

    if (comInitializedHere)
    {
        CoUninitialize();
    }
    return succeeded;
}
