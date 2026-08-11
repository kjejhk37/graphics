# Project Overview

`graphics`는 `World-of-Tank-imitation-Refactoring`(이하 projects)이 git submodule로 참조하는 렌더링 계층 저장소다.

- 렌더러(DirectX 9/11/12, OpenGL)와 UI(ImGui 백엔드 연동)를 담당한다.
- `platform`(다른 세션이 담당하는 저장소)에 의존하지만, `platform`을 submodule로 포함하지 않는다 — `platform`이 `graphics`와 같은 부모 폴더에 형제 디렉터리로 존재한다는 전제로 상대 경로(`GRAPHICS_PLATFORM_DIR` CMake 캐시 변수)를 통해 참조한다.
- projects(app_lib)는 몰라야 한다 — 역방향 의존은 금지.
- 자체 CMake 테스트 빌드/실행 환경(`CMakeLists.txt` + `tests/`)을 가지고 있어, projects의 빌드에 얹혀가지 않고도 단독으로 빌드/테스트할 수 있다.

---

# Shared Workflow & Collaboration Rules

Collaboration principles, the Task Delegation workflow, custom commands, writing rules, and core development principles are defined once in `agent_harness` (linked below as a submodule) and apply here as-is.

@agent_harness/CLAUDE.md
