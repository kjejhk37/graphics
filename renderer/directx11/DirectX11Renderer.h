#pragma once

#include <wrl/client.h>

#include <memory>
#include <vector>

#include <d3d11.h>
#include <dxgi.h>

#include "platform/math/Matrix4x4.h"
#include "graphics/renderer/IRenderer.h"
#include "graphics/ui/directx11/ImGuiManagerDX11.h"

// Author: Claude
// Description: IRenderer의 DirectX 11 구현. ID3D11Device/IDXGISwapChain 등 D3D11 API 호출은 이 클래스(.h/.cpp) 안에만 존재한다.
// Input: 생성자 - forceWarp(테스트용, 기본 false) / Initialize - 렌더링 대상 HWND, 클라이언트 영역 너비/높이
// Output: (해당 없음 - IRenderer 인터페이스 구현)
// Notes: 기본적으로 하드웨어 드라이버로 디바이스 생성을 시도하고, 실패하면 WARP(소프트웨어 래스터라이저)로 폴백한다
//        (GPU/드라이버가 없는 환경에서도 최소한 동작하게 하기 위한 실사용 목적의 폴백 — 테스트 전용이 아니다).
//        forceWarp=true로 생성하면 하드웨어를 건너뛰고 WARP만 시도한다 — 자동 테스트에서 GPU 유무와 무관하게 디바이스 생성을 검증하기 위함.
//        디바이스 상태는 영속 멤버로 보유해 다음 사이클(실제 렌더링 기능)에서 재사용 가능하게 한다.
//        ImGuiManagerDX11을 멤버로 소유해 ImGui 프레임워크(UI 오버레이)를 배선한다 — 이번 사이클은 프레임워크
//        확보만 목표라 실제 위젯은 그리지 않는다.
//        Baseline Vertex/Pixel Shader(shaders/directx11/Baseline.{vs,ps}.cso)로 `MeshManager`가
//        로드한 큐브 모델(assets/models/cube.obj)을 그린다 - 카메라/투영 시스템은 이번 사이클
//        범위 밖이라 WorldViewProj 상수는 Identity를 사용한다(RenderFrame 참고).
//        RenderFrame에 넘어온 snapshot.worldMatrices가 비어있지 않으면 Instancing 파이프라인
//        (INSTANCE_WORLD를 D3D11_INPUT_PER_INSTANCE_DATA로 읽는 입력 슬롯 1)으로 N개 인스턴스를
//        한 번에 그리고, 비어있으면(레거시/스모크 테스트 호출) Baseline 파이프라인으로 큐브
//        하나만 그린다 - 인스턴스 1개짜리 Instancing 드로우와 동등하지만 기존 검증된 경로를
//        그대로 둔다.
// Date: 2026-07-19
class DirectX11Renderer final : public IRenderer
{
public:
    explicit DirectX11Renderer(bool forceWarp = false);

    bool Initialize(HWND windowHandle, int width, int height) override;
    void RenderFrame(const InstanceSnapshot& snapshot) override;
    void OnResize(int width, int height) override;
    void Shutdown() override;
    bool HandleUiMessage(HWND windowHandle, UINT message, WPARAM wParam, LPARAM lParam) override;

    // Author: Claude
    // Description: (테스트 전용) 인스턴스 버퍼(D3D11_USAGE_DYNAMIC, CPU_ACCESS_WRITE만 있어 직접
    //              읽을 수 없음)를 STAGING 리소스로 복사해 CPU에서 읽을 수 있게 한다. 프로덕션
    //              렌더링 경로에서는 호출되지 않는다.
    // Input: (해당 없음)
    // Output: outMatrices - 현재 인스턴스 버퍼에 업로드된 행렬 전체(용량 기준, 마지막 업로드분)
    // Notes: 인스턴스 버퍼가 아직 생성되지 않았으면 false.
    // Date: 2026-07-28
    bool DebugReadBackInstanceBuffer(std::vector<Matrix4x4>& outMatrices) const;

private:
    bool m_forceWarp;
    Microsoft::WRL::ComPtr<ID3D11Device> m_device;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_context;
    Microsoft::WRL::ComPtr<IDXGISwapChain> m_swapChain;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_renderTargetView;
    std::unique_ptr<ImGuiManagerDX11> m_uiManager;

    Microsoft::WRL::ComPtr<ID3D11VertexShader> m_vertexShader;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> m_pixelShader;
    Microsoft::WRL::ComPtr<ID3D11InputLayout> m_inputLayout;
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_positionBuffer;
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_normalBuffer;
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_indexBuffer;
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_constantBuffer;
    UINT m_indexCount = 0;
    UINT m_width = 0;
    UINT m_height = 0;

    Microsoft::WRL::ComPtr<ID3D11VertexShader> m_instancingVertexShader;
    Microsoft::WRL::ComPtr<ID3D11InputLayout> m_instancingInputLayout;
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_instanceBuffer;
    UINT m_instanceBufferCapacity = 0;

    bool CreateDeviceAndSwapChain(D3D_DRIVER_TYPE driverType, HWND windowHandle, int width, int height);
    bool CreateRenderTargetView();
    bool CreateBaselinePipeline();
    bool CreateInstancingPipeline();
    bool EnsureInstanceBufferCapacity(UINT instanceCount);
};
