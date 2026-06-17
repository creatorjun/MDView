# 구현 계획

## 마일스톤 개요

| 마일스톤 | 목표 | 예상 기간 |
|---|---|---|
| M1 | 핵심 렌더링 파이프라인 동작 | 1주 |
| M2 | UI 완성 + 테마 시스템 | 1주 |
| M3 | 파일 와처 + 품질 향상 | 1주 |
| M4 | 배포 패키징 + 스토어 준비 | 1주 |

---

## M1 — 핵심 렌더링 파이프라인

**목표:** `.md` 파일을 열어 WebView2 안에서 아름답게 렌더링한다.

### 환경 세팅

- [ ] Visual Studio 2022 + Windows App SDK 워크로드 설치 확인
- [ ] vcpkg manifest 모드로 `cmark-gfm` 빌드 확인
  - `vcpkg install` 후 `cmark-gfm.h` 인클루드 경로 설정
- [ ] NuGet: `Microsoft.WindowsAppSDK 1.5`, `Microsoft.Web.WebView2 1.0.2535+` 설치
- [ ] `pch.h` 컴파일 검증 (C++20, `/await`, `/bigobj`)

### Domain / Infrastructure

- [ ] `Document` 엔티티 단위 테스트 (Title(), IsLoaded() 검증)
- [ ] `CmarkParser::ToHtml()` 동작 확인
  - GFM 테이블, 체크박스, 취소선, autolink 렌더링 검증
  - 빈 문자열, 유니코드(한글) 입력 경계 케이스
- [ ] `FileRepository::ReadAllText()` UTF-8 / UTF-8 BOM 파일 처리 확인
- [ ] `OpenFileUseCase::Execute()` 통합 테스트

### Presentation

- [ ] `MainWindow::InitWebView()` — `EnsureCoreWebView2Async()` 성공 확인
- [ ] `render-template.html` 로컬 파일 로드 → `NavigateToString()` 렌더링 확인
- [ ] `{{CONTENT}}` 플레이스홀더 치환 로직 동작 확인
- [ ] Pretendard, highlight.js, KaTeX CDN 로드 확인 (인터넷 연결 필요)

**M1 완료 기준:** 실제 `.md` 파일을 Open 버튼으로 열었을 때 WebView2 안에 스타일이 적용된 HTML이 표시된다.

---

## M2 — UI 완성 + 테마 시스템

**목표:** 완성된 WinUI 3 UI, 라이트/다크 테마 완전 동작.

### 커스텀 타이틀바

- [ ] `AppWindow.TitleBar` 커스터마이즈
  - `ExtendsContentIntoTitleBar = true`
  - `SetTitleBar(AppTitleBar)` 호출
  - 드래그 영역 설정 (`SetDragRectangles`)
- [ ] 최소화 / 최대화 / 닫기 버튼 영역 보호 (캡션 버튼 영역 제외)
- [ ] 창 크기 변경 시 타이틀바 레이아웃 재계산

### 테마 시스템

- [ ] `ThemeService::ResolveSystemDark()` 검증 (레지스트리 읽기)
- [ ] WinUI 3 `RequestedTheme` ↔ WebView2 `data-theme` 동기화
  - `ElementTheme::Dark` / `Light` 설정
  - `ExecuteScriptAsync(WebViewThemeScript())` 호출 경로 확인
- [ ] hljs 테마 CSS (`github.min.css` ↔ `github-dark.min.css`) 교체 JS 검증
- [ ] 시스템 테마 변경 감지 (UISettings.ColorValuesChanged 이벤트 구독)

### 드래그 앤 드롭

- [ ] `Window.AllowDrop = true` 설정
- [ ] `DragOver` — `AcceptedOperation::Copy` + 커스텀 캡션
- [ ] `Drop` — `StorageItems` 추출 → `LoadDocument()`
- [ ] 복수 파일 드롭 시 첫 번째 `.md` 파일만 처리
- [ ] 비-마크다운 파일 드롭 시 에러 InfoBar 표시

### 에러 처리 UI

- [ ] `InfoBar` 컨트롤 추가 (XAML)
  - 파일 읽기 실패 (접근 권한, 파일 없음)
  - 렌더링 실패
- [ ] 자동 3초 후 닫힘

**M2 완료 기준:** 라이트/다크 토글이 WinUI 3 UI와 WebView2 렌더링 영역 모두에 즉시 반영된다. 드래그 앤 드롭으로 파일을 열 수 있다.

---

## M3 — 파일 와처 + 품질 향상

**목표:** 파일 저장 시 자동 재로드, 오프라인 렌더링, 스크롤 위치 복원.

### 파일 자동 재로드

- [ ] `FileRepository::WatchFile()` 연결
  - `DispatcherQueue::TryEnqueue()` 로 UI 스레드 마샬링
  - `ViewModel::Reload()` → `NavigateWebView()`
- [ ] 재로드 시 스크롤 위치 복원
  - 재로드 전 `ExecuteScriptAsync("window.scrollY")` → 위치 저장
  - 재로드 후 `NavigationCompleted` 이벤트에서 `window.scrollTo()` 실행
- [ ] 외부 에디터에서 저장하는 시나리오 검증 (VSCode, Notepad++)

### 오프라인 렌더링 (CDN 제거 옵션)

- [ ] highlight.js, KaTeX, Pretendard 폰트를 로컬 Assets에 번들
  - `render-template.html` 에서 CDN URL → 로컬 상대 경로 변경
  - WebView2 `SetVirtualHostNameToFolderMapping()` 으로 가상 호스트 매핑
    ```cpp
    coreWebView2.SetVirtualHostNameToFolderMapping(
        L"mdview.assets",
        assetsPath.wstring(),
        CoreWebView2HostResourceAccessKind::Allow
    );
    ```
  - `render-template.html` 내 URL: `https://mdview.assets/highlight.min.js`

### 외부 링크 처리

- [ ] WebView2 `WebMessageReceived` 이벤트 구독
  - JS에서 `window.chrome.webview.postMessage({ type: 'openUrl', url })` 전송
  - C++에서 `ShellExecuteW(NULL, L"open", url, ...)` 로 기본 브라우저 오픈
- [ ] `NavigationStarting` 이벤트 — 외부 URL 탐색 차단 (보안)

### 최근 파일 메뉴

- [ ] `MenuFlyout` 에 최근 파일 10개 동적 바인딩
- [ ] 앱 재시작 후 복원 (ApplicationData::LocalSettings 저장)
  - 경로가 존재하지 않는 경우 목록에서 제거

### 키보드 단축키

- [ ] `Ctrl+O` — 파일 열기
- [ ] `F5` — 재로드
- [ ] `Ctrl+Shift+T` — 테마 토글
- [ ] `Escape` — 전체화면 해제
- [ ] `KeyboardAccelerator` XAML 바인딩

**M3 완료 기준:** 외부 에디터에서 파일 저장 시 0.5~1초 이내 자동 재로드된다. 인터넷 없이도 렌더링이 완전히 동작한다.

---

## M4 — 배포 패키징

**목표:** MSIX 패키지 배포, Windows 스토어 제출 준비.

### MSIX 패키징

- [ ] `Package.appxmanifest` 최종 검토
  - 파일 연결 (`.md`, `.markdown`, `.mdown`) 동작 확인
  - 프로토콜 등록 (선택 사항: `mdview://` 딥링크)
- [ ] 앱 아이콘 에셋 생성 (44x44, 150x150, 310x150, StoreLogo)
  - WinUI 3 에셋 스케일 (`scale-100`, `scale-125`, `scale-150`, `scale-200`)
- [ ] 코드 서명 인증서 설정

### 인스톨러 / 배포

- [ ] GitHub Actions CI 설정
  - `msbuild MDView.sln /p:Configuration=Release /p:Platform=x64`
  - MSIX 패키지 아티팩트 업로드
- [ ] 무서명 사이드로딩 가이드 (README 업데이트)

### 성능 최적화

- [ ] 시작 시간 프로파일링 (목표: 콜드 스타트 < 500ms)
- [ ] 대용량 파일 (1MB+) 파싱 시간 측정
- [ ] WebView2 프로세스 격리 옵션 검토

**M4 완료 기준:** `mdview-setup.msix` 로 클린 설치 후 `.md` 파일 더블클릭으로 앱이 열린다.
