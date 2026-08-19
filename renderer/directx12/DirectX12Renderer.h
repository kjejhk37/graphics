#pragma once

#include <wrl/client.h>

#include <array>
#include <memory>
#include <vector>

#include <d3d12.h>
#include <dxgi1_4.h>

#include "platform/math/Matrix4x4.h"
#include "graphics/renderer/IRenderer.h"
#include "graphics/ui/directx12/ImGuiManagerDX12.h"

class IUiElementRegistry;

// Author: Claude
// Description: IRenderer의 DirectX 12 구현. ID3D12Device 등 D3D12 API 호출은 이 클래스(.h/.cpp) 안에만 존재한다.
// Input: 생성자 - forceWarp(테스트용, 기본 false) / Initialize - 렌더링 대상 HWND, 클라이언트 영역 너비/높이
// Output: (해당 없음 - IRenderer 인터페이스 구현)
// Notes: 기본적으로 하드웨어 어댑터로 디바이스 생성을 시도하고, 실패하면 WARP 어댑터로 폴백한다(DX11과 동일한 실사용 목적 폴백).
//        forceWarp=true면 하드웨어를 건너뛰고 WARP 어댑터만 시도 — 자동 테스트에서 GPU 유무와 무관하게 디바이스 생성을 검증하기 위함.
//        kCommandListCount는 확장성 가이드라인(브레인스토밍 참고) — 지금은 1(싱글스레드 기록)이지만, 나중에 커맨드 리스트를
//        여러 스레드에서 병렬 기록하도록 확장할 때 이 상수와 관련 컨테이너 크기만 늘리면 되게 구조를 열어둔다.
//        매 프레임 Present 직후 펜스로 GPU를 완전히 대기(WaitForGpu)한다 — 프레임 파이프라이닝 최적화는 이번 사이클 범위 밖.
//        ImGuiManagerDX12를 멤버로 소유해 ImGui 프레임워크(UI 오버레이)를 배선한다 — 이번 사이클은 프레임워크
//        확보만 목표라 실제 위젯은 그리지 않는다. IUiManager*가 아니라 구체 타입으로 들고 있는 이유는
//        RenderWithCommandList(ID3D12GraphicsCommandList*)가 인터페이스 밖 DX12 전용 메서드이기 때문이다.
//        Baseline Vertex/Pixel Shader(shaders/directx12/Baseline.{vs,ps}.cso)로 `MeshManager`가
//        로드한 큐브 모델(assets/models/cube.obj)을 그린다 - 카메라/투영 시스템은 이번 사이클
//        범위 밖이라 WorldViewProj 상수는 Identity를 사용한다(RenderFrame 참고).
//        정점(position/normal)/인덱스/상수 버퍼 전부 D3D12_HEAP_TYPE_UPLOAD에 커밋 리소스로 만들고
//        CPU에서 직접 매핑해 채운다 - 정적이고 크기가 작은 데이터라 DEFAULT 힙 + 복사
//        커맨드리스트를 두는 것보다 단순하다.
//        루트 시그니처는 CBV 디스크립터 테이블 대신 루트 CBV 파라미터 하나만 사용한다 - 디스크립터
//        힙 없이 SetGraphicsRootConstantBufferView로 바로 바인딩할 수 있어 이번 사이클 범위(단일
//        상수 버퍼)에는 더 단순하다.
//        RenderFrame에 넘어온 snapshot.worldMatrices가 비어있지 않으면 Instancing PSO(INSTANCE_WORLD를
//        D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA로 읽는 입력 슬롯 1)로 N개 인스턴스를 한 번에
//        그리고, 비어있으면 Baseline PSO로 큐브 하나만 그린다(DX11과 동일한 이유).
//        SetUiElementRegistry로 등록된 IUiElementRegistry는 RenderFrame 내부에서 m_uiManager->NewFrame()
//        직후, RenderWithCommandList 이전에 RenderAll()이 호출된다 - 등록 안 됐으면(nullptr) 아무 일도 하지 않는다.
// Date: 2026-07-19
class DirectX12Renderer final : public IRenderer
{
public:
    explicit DirectX12Renderer(bool forceWarp = false);
    ~DirectX12Renderer() override;

    bool Initialize(HWND windowHandle, int width, int height) override;
    void RenderFrame(const InstanceSnapshot& snapshot) override;
    void OnResize(int width, int height) override;
    void Shutdown() override;
    bool HandleUiMessage(HWND windowHandle, UINT message, WPARAM wParam, LPARAM lParam) override;
    void SetUiElementRegistry(IUiElementRegistry& registry) override;

    // Author: Claude
    // Description: (테스트 전용) 인스턴스 버퍼(업로드 힙, 프로덕션에서도 매핑 유지)의 현재 내용을
    //              읽어온다. 프로덕션 렌더링 경로에서는 호출되지 않는다.
    // Input: (해당 없음)
    // Output: outMatrices - 현재 인스턴스 버퍼에 업로드된 행렬 전체(용량 기준, 마지막 업로드분)
    // Notes: 인스턴스 버퍼가 아직 생성되지 않았으면 false.
    // Date: 2026-07-28
    bool DebugReadBackInstanceBuffer(std::vector<Matrix4x4>& outMatrices) const;

private:
    static constexpr UINT kBackBufferCount = 2;   // DX12 flip-model 스왑체인의 최소 버퍼 요구사항
    static constexpr UINT kCommandListCount = 1;  // 확장성 가이드라인 — 나중에 멀티스레드 기록 시 이 값만 늘리면 됨

    bool m_forceWarp;
    Microsoft::WRL::ComPtr<ID3D12Device> m_device;
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_commandQueue;
    Microsoft::WRL::ComPtr<IDXGISwapChain3> m_swapChain;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
    std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, kBackBufferCount> m_renderTargets;
    std::array<Microsoft::WRL::ComPtr<ID3D12CommandAllocator>, kCommandListCount> m_commandAllocators;
    std::array<Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>, kCommandListCount> m_commandLists;
    Microsoft::WRL::ComPtr<ID3D12Fence> m_fence;
    HANDLE m_fenceEvent = nullptr;
    UINT64 m_fenceValue = 0;
    UINT m_rtvDescriptorSize = 0;
    UINT m_frameIndex = 0;
    UINT m_width = 0;
    UINT m_height = 0;
    std::unique_ptr<ImGuiManagerDX12> m_uiManager;
    IUiElementRegistry* m_uiElementRegistry = nullptr;

    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pipelineState;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_positionBuffer;
    D3D12_VERTEX_BUFFER_VIEW m_positionBufferView{};
    Microsoft::WRL::ComPtr<ID3D12Resource> m_normalBuffer;
    D3D12_VERTEX_BUFFER_VIEW m_normalBufferView{};
    Microsoft::WRL::ComPtr<ID3D12Resource> m_indexBuffer;
    D3D12_INDEX_BUFFER_VIEW m_indexBufferView{};
    UINT m_indexCount = 0;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_constantBuffer;
    void* m_constantBufferMappedData = nullptr;

    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_instancingPipelineState;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_instanceBuffer;
    void* m_instanceBufferMappedData = nullptr;
    UINT m_instanceBufferCapacity = 0;

    bool CreateDevice();
    bool CreateSwapChain(HWND windowHandle, int width, int height);
    bool CreateRenderTargets();
    void ReleaseRenderTargets();
    void WaitForGpu();
    bool CreateBaselinePipeline();
    bool CreateInstancingPipeline();
    bool EnsureInstanceBufferCapacity(UINT instanceCount);
};
