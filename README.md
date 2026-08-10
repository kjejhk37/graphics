# graphics

[World-of-Tank-imitation-Refactoring](https://github.com/kjejhk37/World-of-Tank-imitation-Refactoring)의 `src/graphics` 계층을 독립 저장소로 분리한 것입니다.
해당 프로젝트에는 git submodule로 연결되어 있습니다.

## 역할

platform을 사용해 DirectX/OpenGL 등 특정 그래픽스 API를 캡슐화하는 계층입니다.
Unity/Unreal 같은 "엔진"에 해당하는 위치로, 시각화(View)와 렌더링 파이프라인을 책임집니다.

- `projects`(WOT 프로젝트의 실제 콘텐츠/규칙)를 몰라야 합니다. `platform`에는 의존해도 됩니다.
- 공개 인터페이스(`IRenderer` 등)가 주고받는 데이터 타입의 모양은 이 계층이 직접 정의하고 소유합니다.
- 탱크, 맵 이름 등 WOT 고유 개념은 포함하지 않습니다 — 다른 프로젝트에서도 재사용 가능해야 합니다.

계층 판단 기준의 전체 맥락은 상위 프로젝트의 `docs/architecture/platform_graphics_projects.md`를 참고하세요.

## 구성

- `renderer/` — 렌더러 인터페이스(`IRenderer`), 백엔드별 구현(DirectX9/11/12, OpenGL), 셰이더.
- `ui/` — 렌더러 백엔드별 ImGui 연동.

## 개발 환경

- 언어: C++
- 빌드 시스템: 상위 프로젝트의 CMake 빌드에 편입되어 빌드됩니다(단독 빌드 스크립트 없음).

협업 방식과 워크플로우는 [`agent_harness/CLAUDE.md`](./agent_harness/CLAUDE.md)를 따릅니다.
