cmake_minimum_required(VERSION 3.20)

project(Graphics LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    # 소스 내 한글 주석이 로컬 코드페이지(949)로 오인되어 C4819가 나는 것을 방지 — 소스/실행 문자셋을 UTF-8로 고정.
    add_compile_options(/utf-8)
endif()

include(FetchContent)

# platform 저장소는 graphics의 형제 폴더로 존재한다는 전제(같은 부모 폴더)로 상대 경로 참조한다.
# 위치가 바뀌면 이 캐시 변수만 오버라이드하면 된다.
set(GRAPHICS_PLATFORM_DIR "${CMAKE_SOURCE_DIR}/../platform" CACHE PATH
    "Path to the platform repository root (sibling checkout by default)")

# --- FetchContent 5종 ---
# World-of-Tank-imitation-Refactoring/CMakeLists.txt와 동일 버전 유지 필요 — 버전이 어긋나면
# graphics/projects 간 빌드 결과가 달라질 수 있다.

# nlohmann::json은 JsonDataStore 구현(platform_lib 내부)에서만 쓰인다.
FetchContent_Declare(
    nlohmann_json
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(nlohmann_json)

# Dear ImGui는 소비자용 CMakeLists.txt를 제공하지 않는다 — 소스만 받아 graphics_lib에 직접 추가한다.
FetchContent_Declare(
    imgui
    URL https://github.com/ocornut/imgui/archive/refs/tags/v1.92.8.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(imgui)

# tinyobjloader v1.0.7의 CMakeLists.txt가 cmake_minimum_required(VERSION 2.8.11)을 선언하는데,
# 최신 CMake(4.x)는 3.5 미만의 호환성을 완전히 제거했다 - 이 서브프로젝트에 한해 정책 최소 버전을 낮춰준다.
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
FetchContent_Declare(
    tinyobjloader
    URL https://github.com/tinyobjloader/tinyobjloader/archive/refs/tags/v1.0.7.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(tinyobjloader)
unset(CMAKE_POLICY_VERSION_MINIMUM)

# ufbx는 단일 소스 라이브러리다 — 소스만 받아 ufbx.c를 platform_lib에 직접 추가한다.
FetchContent_Declare(
    ufbx
    URL https://github.com/ufbx/ufbx/archive/refs/tags/v0.23.0.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(ufbx)

# cgltf는 단일 헤더 라이브러리다 - CgltfWrapper.cpp 안에서 구현부를 생성하므로 include 경로만 추가한다.
FetchContent_Declare(
    cgltf
    URL https://github.com/jkuhlmann/cgltf/archive/refs/tags/v1.15.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(cgltf)

# --- include 경로 해석 ---
# graphics/platform 저장소 모두 자기 자신을 가리키는 접두어로 include한다.
# (예: graphics/renderer/RendererFactory.cpp 안에서 #include "graphics/renderer/RendererFactory.h")
# 각 저장소의 "부모 폴더"를 include 루트로 잡아야 이 접두어 규칙이 성립한다.
get_filename_component(_graphics_parent_dir "${CMAKE_SOURCE_DIR}" DIRECTORY)
get_filename_component(_platform_parent_dir "${GRAPHICS_PLATFORM_DIR}" DIRECTORY)

# platform_lib는 어떤 상위 타겟도 링크하지 않는다(platform은 graphics를 몰라야 함).
# 아래 소스 목록은 World-of-Tank-imitation-Refactoring/CMakeLists.txt의 platform_lib 목록과
# 동기화 필요 — 파일 추가/삭제 시 두 곳을 함께 갱신할 것.
add_library(platform_lib STATIC
    ${GRAPHICS_PLATFORM_DIR}/logging/LogFormatter.cpp
    ${GRAPHICS_PLATFORM_DIR}/logging/ConsoleLogSink.cpp
    ${GRAPHICS_PLATFORM_DIR}/logging/FileLogSink.cpp
    ${GRAPHICS_PLATFORM_DIR}/logging/Logger.cpp
    ${GRAPHICS_PLATFORM_DIR}/logging/Log.cpp
    ${GRAPHICS_PLATFORM_DIR}/Win32Window.cpp
    ${GRAPHICS_PLATFORM_DIR}/serialization/DataRecordAccess.cpp
    ${GRAPHICS_PLATFORM_DIR}/serialization/JsonDataStore.cpp
    ${GRAPHICS_PLATFORM_DIR}/serialization/SqliteDataStore.cpp
    ${GRAPHICS_PLATFORM_DIR}/persistence/SaveLoadManager.cpp
    ${GRAPHICS_PLATFORM_DIR}/math/Matrix4x4.cpp
    ${GRAPHICS_PLATFORM_DIR}/geometry/Intersections.cpp
    ${GRAPHICS_PLATFORM_DIR}/geometry/Geometry.cpp
    ${GRAPHICS_PLATFORM_DIR}/collision/gjk_epa/Gjk.cpp
    ${GRAPHICS_PLATFORM_DIR}/collision/gjk_epa/Epa.cpp
    ${GRAPHICS_PLATFORM_DIR}/collision/gjk_epa/GjkEpa.cpp
    ${GRAPHICS_PLATFORM_DIR}/collision/sweep_and_prune/SweepAndPrune.cpp
    ${GRAPHICS_PLATFORM_DIR}/collision/bvh/Bvh.cpp
    ${GRAPHICS_PLATFORM_DIR}/collision/rtree/RTree.cpp
    ${GRAPHICS_PLATFORM_DIR}/collision/voxel_grid/VoxelGrid.cpp
    ${GRAPHICS_PLATFORM_DIR}/collision/spatial_hash/SpatialHash.cpp
    ${GRAPHICS_PLATFORM_DIR}/collision/octree/Octree.cpp
    ${GRAPHICS_PLATFORM_DIR}/collision/kdtree/KdTree.cpp
    ${GRAPHICS_PLATFORM_DIR}/collision/mpr/Mpr.cpp
    ${GRAPHICS_PLATFORM_DIR}/collision/conservative_advancement/ConservativeAdvancement.cpp
    ${GRAPHICS_PLATFORM_DIR}/collision/fcl/Fcl.cpp
    ${GRAPHICS_PLATFORM_DIR}/entity_component/Entity.cpp
    ${GRAPHICS_PLATFORM_DIR}/entity_component/EntityManager.cpp
    ${GRAPHICS_PLATFORM_DIR}/model_import/TinyObjWrapper.cpp
    ${GRAPHICS_PLATFORM_DIR}/model_import/UfbxWrapper.cpp
    ${GRAPHICS_PLATFORM_DIR}/model_import/CgltfWrapper.cpp
    ${GRAPHICS_PLATFORM_DIR}/model_import/ModelLoader.cpp
    ${GRAPHICS_PLATFORM_DIR}/model_import/RefCountingCachePolicy.cpp
    ${GRAPHICS_PLATFORM_DIR}/model_import/LruCachePolicy.cpp
    ${GRAPHICS_PLATFORM_DIR}/model_import/Win32FileWatcher.cpp
    ${GRAPHICS_PLATFORM_DIR}/model_import/HotReloadCachePolicy.cpp
    ${GRAPHICS_PLATFORM_DIR}/model_import/NaiveSignatureDeduplicator.cpp
    ${GRAPHICS_PLATFORM_DIR}/model_import/MeshManager.cpp
    ${ufbx_SOURCE_DIR}/ufbx.c
)
target_include_directories(platform_lib PUBLIC ${_platform_parent_dir})
target_include_directories(platform_lib PRIVATE ${ufbx_SOURCE_DIR} ${cgltf_SOURCE_DIR})
target_link_libraries(platform_lib PRIVATE nlohmann_json::nlohmann_json tinyobjloader)

# graphics_lib는 platform_lib에는 의존해도 되지만 projects(app_lib)는 몰라야 한다.
# 아래 소스 목록은 World-of-Tank-imitation-Refactoring/CMakeLists.txt의 graphics_lib 목록과
# 동기화 필요 — 파일 추가/삭제 시 두 곳을 함께 갱신할 것.
add_library(graphics_lib STATIC
    ${CMAKE_SOURCE_DIR}/renderer/RendererFactory.cpp
    ${CMAKE_SOURCE_DIR}/renderer/RendererBackendParser.cpp
    ${CMAKE_SOURCE_DIR}/renderer/ShaderBytecodeLoader.cpp
    ${CMAKE_SOURCE_DIR}/renderer/directx9/DirectX9Renderer.cpp
    ${CMAKE_SOURCE_DIR}/renderer/directx11/DirectX11Renderer.cpp
    ${CMAKE_SOURCE_DIR}/renderer/directx11/DirectX11ComputeDemo.cpp
    ${CMAKE_SOURCE_DIR}/renderer/directx12/DirectX12Renderer.cpp
    ${CMAKE_SOURCE_DIR}/renderer/directx12/DirectX12ComputeDemo.cpp
    ${CMAKE_SOURCE_DIR}/renderer/opengl/OpenGLRenderer.cpp
    ${CMAKE_SOURCE_DIR}/ui/directx9/ImGuiManagerDX9.cpp
    ${CMAKE_SOURCE_DIR}/ui/directx11/ImGuiManagerDX11.cpp
    ${CMAKE_SOURCE_DIR}/ui/directx12/ImGuiManagerDX12.cpp
    ${CMAKE_SOURCE_DIR}/ui/widgets/UiElementRegistry.cpp
    ${CMAKE_SOURCE_DIR}/ui/widgets/ButtonUI.cpp
    ${CMAKE_SOURCE_DIR}/ui/widgets/ProgressUI.cpp
    ${CMAKE_SOURCE_DIR}/ui/widgets/StringFieldUI.cpp
    ${CMAKE_SOURCE_DIR}/ui/widgets/LabelUI.cpp
    ${CMAKE_SOURCE_DIR}/ui/widgets/ImageUI.cpp
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_dx9.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_dx11.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_dx12.cpp
)
target_include_directories(graphics_lib PUBLIC ${_graphics_parent_dir} ${_platform_parent_dir})
target_include_directories(graphics_lib PRIVATE ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)
target_link_libraries(graphics_lib PUBLIC platform_lib)

# DirectX 9/11/12는 Windows SDK에 포함된 시스템 라이브러리라 FetchContent 대상이 아니다.
target_link_libraries(graphics_lib PUBLIC d3d9.lib d3d11.lib d3d12.lib dxgi.lib)

# --- 셰이더 빌드 파이프라인 (오프라인 컴파일, fxc.exe) ---
# 배포 시 D3DCompiler_47.dll 의존성을 없애기 위해 HLSL을 런타임이 아닌 빌드 시점에 fxc.exe로 컴파일한다.
# World-of-Tank-imitation-Refactoring/CMakeLists.txt의 wot_add_hlsl_shader()를 이식한 뒤 graphics 저장소
# 맥락에 맞게 접두어를 wot_ -> graphics_로 재명명했다.
# fxc.exe는 Windows SDK에 포함되지만 PATH에 없는 경우가 많아, 설치된 모든 SDK 버전의 x64 디렉터리를
# 검색 경로 후보로 넘겨 find_program이 찾게 한다.
file(GLOB _graphics_fxc_candidates "C:/Program Files (x86)/Windows Kits/10/bin/*/x64/fxc.exe")
set(_graphics_fxc_search_dirs "")
foreach(_graphics_fxc_candidate ${_graphics_fxc_candidates})
    get_filename_component(_graphics_fxc_dir "${_graphics_fxc_candidate}" DIRECTORY)
    list(APPEND _graphics_fxc_search_dirs "${_graphics_fxc_dir}")
endforeach()

find_program(FXC_EXECUTABLE fxc PATHS ${_graphics_fxc_search_dirs})
if(NOT FXC_EXECUTABLE)
    message(FATAL_ERROR "fxc.exe(HLSL 오프라인 셰이더 컴파일러)를 찾을 수 없습니다 - Windows SDK 설치를 확인하세요.")
endif()

# 셰이더 소스는 백엔드별 폴더(renderer/<backend>/shaders/<name>.hlsl)에 두고, <backend>는 그 경로에서
# 자동으로 추론한다. 컴파일된 .cso는 tests.exe와 같은 위치(<config>/shaders/<backend>/)에 출력해,
# 테스트가 상대 경로로 바로 로드할 수 있게 한다. graphics 저장소에는 projects의 main.exe에 해당하는
# 실행 파일이 없으므로 tests 타겟에만 의존시킨다(아래 shaders 커스텀 타겟 참고).
set(GRAPHICS_SHADER_OUTPUTS "")

# Author: graphic 세션 (World-of-Tank-imitation-Refactoring/CMakeLists.txt에서 이식)
# Description: HLSL 셰이더 소스 하나를 fxc.exe로 오프라인 컴파일해 .cso 바이트코드를 산출하는
#              add_custom_command를 등록한다.
# Input:
#   OUT_VAR - 산출된 .cso 파일의 절대 경로를 담아 호출부로 돌려줄 출력 변수명
#   SOURCE - 컴파일할 .hlsl 파일의 절대 경로 (renderer/<backend>/shaders/<name>.hlsl 형태 가정)
#   ENTRY_POINT - HLSL 진입점 함수명 (예: VSMain, PSMain, CSMain)
#   PROFILE - fxc 셰이더 프로파일 (예: vs_5_0, ps_5_0, cs_5_0)
# Output: ${OUT_VAR}를 PARENT_SCOPE에 설정 — 산출 .cso 경로 문자열.
# Notes:
#   - NAME_WE는 "가장 긴 확장자"를 통째로 잘라내므로 "Baseline.vs.hlsl" -> "Baseline"이 되어 vs/ps
#     두 셰이더가 같은 출력 파일명으로 충돌한다 - 마지막 ".hlsl" 확장자만 정규식으로 제거해
#     "Baseline.vs"/"Baseline.ps"를 그대로 보존한다.
#   - $<TARGET_FILE_DIR:...> 같은 "다른 타겟을 참조하는" 제너레이터 표현식은 Visual Studio 멀티
#     컨피그 생성기에서 add_custom_command의 OUTPUT 경로로 쓰면 "No target ..." 생성 오류가 난다 -
#     tests.exe가 실제로 출력되는 위치(${CMAKE_BINARY_DIR}/$<CONFIG>/)를 $<CONFIG>만 사용해 직접
#     구성한다.
# Date: 2026-08-11
function(graphics_add_hlsl_shader OUT_VAR SOURCE ENTRY_POINT PROFILE)
    get_filename_component(_shader_full_name "${SOURCE}" NAME)
    string(REGEX REPLACE "\\.hlsl$" "" _shader_name "${_shader_full_name}")
    get_filename_component(_shaders_dir "${SOURCE}" DIRECTORY)
    get_filename_component(_backend_dir "${_shaders_dir}" DIRECTORY)
    get_filename_component(_backend "${_backend_dir}" NAME)

    set(_output_dir "${CMAKE_BINARY_DIR}/$<CONFIG>/shaders/${_backend}")
    set(_output_file "${_output_dir}/${_shader_name}.cso")

    add_custom_command(
        OUTPUT "${_output_file}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${_output_dir}"
        COMMAND ${FXC_EXECUTABLE} /nologo /T ${PROFILE} /E ${ENTRY_POINT} /Fo "${_output_file}" "${SOURCE}"
        DEPENDS "${SOURCE}"
        COMMENT "HLSL: ${SOURCE} (${PROFILE}, entry=${ENTRY_POINT}) -> ${_output_file}"
        VERBATIM
    )

    set(${OUT_VAR} "${_output_file}" PARENT_SCOPE)
endfunction()

# 셰이더 하나를 추가/삭제할 때 이 목록에 "경로|entry|profile" 한 줄만 손대면 되도록, 개별
# graphics_add_hlsl_shader() 호출을 데이터 목록 + foreach 루프로 대체한다. backend는
# graphics_add_hlsl_shader() 내부에서 경로로부터 자동 추론되므로 목록에는 포함하지 않는다.
set(_graphics_shader_specs
    "renderer/directx9/shaders/Baseline.vs.hlsl|VSMain|vs_3_0"
    "renderer/directx9/shaders/Baseline.ps.hlsl|PSMain|ps_3_0"
    "renderer/directx11/shaders/Baseline.vs.hlsl|VSMain|vs_5_0"
    "renderer/directx11/shaders/Baseline.ps.hlsl|PSMain|ps_5_0"
    "renderer/directx12/shaders/Baseline.vs.hlsl|VSMain|vs_5_0"
    "renderer/directx12/shaders/Baseline.ps.hlsl|PSMain|ps_5_0"
    "renderer/directx9/shaders/Instancing.vs.hlsl|VSMain|vs_3_0"
    "renderer/directx11/shaders/Instancing.vs.hlsl|VSMain|vs_5_0"
    "renderer/directx12/shaders/Instancing.vs.hlsl|VSMain|vs_5_0"
    "renderer/directx11/shaders/Transform.cs.hlsl|CSMain|cs_5_0"
    "renderer/directx12/shaders/Transform.cs.hlsl|CSMain|cs_5_0"
)
foreach(_spec ${_graphics_shader_specs})
    string(REPLACE "|" ";" _spec_fields "${_spec}")
    list(GET _spec_fields 0 _rel_source)
    list(GET _spec_fields 1 _entry_point)
    list(GET _spec_fields 2 _profile)
    graphics_add_hlsl_shader(_shader_output "${CMAKE_SOURCE_DIR}/${_rel_source}" ${_entry_point} ${_profile})
    list(APPEND GRAPHICS_SHADER_OUTPUTS ${_shader_output})
endforeach()

# GoogleTest는 tests 타겟에만 링크한다 — graphics_lib/platform_lib은 테스트 프레임워크에 의존하지 않는다.
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

# World-of-Tank-imitation-Refactoring/tests/에서 복사한 파일 — 원본은 그대로 유지된다.
# 1단계: 디바이스/셰이더 자산 없이 도는 순수 로직 테스트.
# 2단계: 실제 WARP/D3D 디바이스 초기화와 컴파일된 .cso 셰이더/모델 픽스처를 요구하는 테스트.
add_executable(tests
    tests/RendererBackendParserTest.cpp
    tests/RendererFactoryTest.cpp
    tests/ShaderBytecodeLoaderTest.cpp
    tests/DirectX9RendererTest.cpp
    tests/DirectX11RendererTest.cpp
    tests/DirectX12RendererTest.cpp
    tests/ImGuiManagerDX9Test.cpp
    tests/ImGuiManagerDX11Test.cpp
    tests/ImGuiManagerDX12Test.cpp
    tests/DirectX11ComputeDemoTest.cpp
    tests/DirectX12ComputeDemoTest.cpp
    tests/UiElementRegistryTest.cpp
    tests/UiWidgetsTest.cpp
)
target_link_libraries(tests PRIVATE graphics_lib platform_lib gtest_main)
target_compile_definitions(tests PRIVATE GRAPHICS_SHADER_OUTPUT_DIR="${CMAKE_BINARY_DIR}/$<CONFIG>/shaders/")

include(GoogleTest)
gtest_discover_tests(tests WORKING_DIRECTORY "$<TARGET_FILE_DIR:tests>")

# 이 시점까지 등록된 모든 graphics_add_hlsl_shader() 호출의 .cso 출력을 하나의 커스텀 타겟으로 묶는다 —
# tests가 셰이더가 빌드된 뒤에 실행되어야 하므로 의존시킨다.
add_custom_target(shaders DEPENDS ${GRAPHICS_SHADER_OUTPUTS})
add_dependencies(tests shaders)

# DirectX9/11/12Renderer::Initialize()가 상대 경로 "assets/models/cube.obj"로 큐브 모델을 읽으므로,
# 실행 파일 옆에 복사해 둔다.
add_custom_command(TARGET tests POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:tests>/assets/models"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/tests/fixtures/model_import/cube.obj"
        "$<TARGET_FILE_DIR:tests>/assets/models/cube.obj"
    COMMENT "Copying cube.obj next to tests.exe"
)
