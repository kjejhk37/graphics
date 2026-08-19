#pragma once

#include <wrl/client.h>

#include <memory>
#include <string>
#include <vector>

#include <d3d9.h>

#include "platform/math/Matrix4x4.h"
#include "graphics/renderer/IRenderer.h"
#include "graphics/ui/directx9/ImGuiManagerDX9.h"

class IUiElementRegistry;

// Author: Claude
// Description: IRenderer의 DirectX 9 구현. IDirect3D9/IDirect3DDevice9 API 호출은 이 클래스(.h/.cpp) 안에만 존재한다.
// Input: 생성자 - forceInstancingFallback(테스트용, 기본 false) / Initialize - 렌더링 대상 HWND, 클라이언트
//        영역 너비/높이
// Output: (해당 없음 - IRenderer 인터페이스 구현)
// Notes: DX9은 DXGI를 쓰지 않는 세대라 D3DPRESENT_PARAMETERS로 암시적 스왑체인을 구성한다.
//        DX11/12의 WARP와 달리, DX9의 REF/NULLREF 소프트웨어 디바이스는 별도 레거시 SDK 컴포넌트가 있어야 안정적으로 동작해
//        이 클래스는 하드웨어(D3DDEVTYPE_HAL) 생성만 시도한다 — 실패 시 false를 반환한다.
//        디바이스 상태(IDirect3D9/IDirect3DDevice9)는 영속 멤버로 보유해 다음 사이클(실제 렌더링 기능)에서 재사용 가능하게 한다.
//        ImGuiManagerDX9을 멤버로 소유해 ImGui 프레임워크(UI 오버레이)를 배선한다 — 이번 사이클은 프레임워크
//        확보만 목표라 실제 위젯은 그리지 않는다(Render는 매 프레임 호출되지만 그 사이에 그려지는 콘텐츠 없음).
//        Baseline Vertex/Pixel Shader(shaders/directx9/Baseline.{vs,ps}.cso)로 `MeshManager`가
//        로드한 큐브 모델(assets/models/cube.obj)을 그린다 - 카메라/투영 시스템은 이번 사이클
//        범위 밖이라 WorldViewProj 상수는 Identity를 사용한다(RenderFrame 참고, 모델이 클립 공간
//        범위를 벗어나면 화면 일부만 보일 수 있음 - 카메라 시스템은 다음 사이클 범위).
//        RenderFrame에 넘어온 snapshot.worldMatrices가 비어있지 않으면 SetStreamSourceFreq
//        (D3DSTREAMSOURCE_INDEXEDDATA/INSTANCEDATA)로 하드웨어 인스턴싱을 시도한다 - Direct3D 9엔
//        이를 사전에 질의할 캡ability 플래그가 없어, 매 프레임 실제로 시도한 뒤 반환값으로만 성공
//        여부를 판단한다. 실패하면(또는 forceInstancingFallback=true로 강제하면) Baseline
//        파이프라인을 인스턴스 개수만큼 반복 호출하는 CPU 폴백으로 전환한다(DrawInstances 참고).
//        SetUiElementRegistry로 등록된 IUiElementRegistry는 RenderFrame 내부에서 m_uiManager->NewFrame()
//        직후, m_uiManager->Render() 이전에 RenderAll()이 호출된다 - 등록 안 됐으면(nullptr) 아무 일도 하지 않는다.
// Date: 2026-07-19
class DirectX9Renderer final : public IRenderer
{
public:
    explicit DirectX9Renderer(bool forceInstancingFallback = false);

    bool Initialize(HWND windowHandle, int width, int height) override;
    void RenderFrame(const InstanceSnapshot& snapshot) override;
    void OnResize(int width, int height) override;
    void Shutdown() override;
    bool HandleUiMessage(HWND windowHandle, UINT message, WPARAM wParam, LPARAM lParam) override;
    void SetUiElementRegistry(IUiElementRegistry& registry) override;

    // Author: Claude
    // Description: 스텁 - 항상 nullptr을 반환한다. DX9은 WOT의 실사용 백엔드가 아니라(기본은 DX11)
    //              이번 사이클에서 실제 텍스처 로딩을 구현하지 않았다 - DX9이 실사용되는 시점에
    //              새 요청으로 확장한다(IRenderer.h Notes 참고).
    // Input: filePath - (사용 안 함)
    // Output: 항상 nullptr.
    // Notes: IRenderer 계약(컴파일 통과) 만족 목적.
    // Date: 2026-08-19
    void* LoadTexture(const std::string& filePath) override;
    void UnloadTexture(void* textureHandle) override;

    // Author: Claude
    // Description: (테스트 전용) 인스턴스 버퍼의 현재 내용을 읽어온다. 프로덕션 렌더링 경로에서는
    //              호출되지 않는다.
    // Input: (해당 없음)
    // Output: outMatrices - 현재 인스턴스 버퍼에 업로드된 행렬 전체
    // Notes: 인스턴스 버퍼가 아직 생성되지 않았으면(폴백 경로만 탄 경우 포함) false.
    // Date: 2026-07-28
    bool DebugReadBackInstanceBuffer(std::vector<Matrix4x4>& outMatrices) const;

private:
    bool m_forceInstancingFallback;
    Microsoft::WRL::ComPtr<IDirect3D9> m_direct3D;
    Microsoft::WRL::ComPtr<IDirect3DDevice9> m_device;
    HWND m_windowHandle = nullptr;
    std::unique_ptr<ImGuiManagerDX9> m_uiManager;
    IUiElementRegistry* m_uiElementRegistry = nullptr;

    Microsoft::WRL::ComPtr<IDirect3DVertexShader9> m_vertexShader;
    Microsoft::WRL::ComPtr<IDirect3DPixelShader9> m_pixelShader;
    Microsoft::WRL::ComPtr<IDirect3DVertexDeclaration9> m_vertexDeclaration;
    Microsoft::WRL::ComPtr<IDirect3DVertexBuffer9> m_positionBuffer;
    Microsoft::WRL::ComPtr<IDirect3DVertexBuffer9> m_normalBuffer;
    Microsoft::WRL::ComPtr<IDirect3DIndexBuffer9> m_indexBuffer;
    UINT m_vertexCount = 0;
    UINT m_indexCount = 0;

    Microsoft::WRL::ComPtr<IDirect3DVertexShader9> m_instancingVertexShader;
    Microsoft::WRL::ComPtr<IDirect3DVertexDeclaration9> m_instancingVertexDeclaration;
    Microsoft::WRL::ComPtr<IDirect3DVertexBuffer9> m_instanceBuffer;
    UINT m_instanceBufferCapacity = 0;

    bool CreateBaselinePipeline();
    bool CreateInstancingPipeline();
    bool EnsureInstanceBufferCapacity(UINT instanceCount);
    void DrawInstances(const std::vector<Matrix4x4>& worldMatrices);
};
