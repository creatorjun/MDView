# 아키텍처 설계

## 1. 설계 철학

MDView는 **Clean Architecture (Hexagonal Architecture)** 원칙을 엄격하게 적용한다.
비즈니스 로직(Domain)은 UI 프레임워크, 파일 시스템, 파싱 라이브러리 등 모든 외부 의존성으로부터 완전히 격리된다.
외부 세계는 인터페이스(Port)를 통해서만 Domain과 소통한다.

```
┌──────────────────────────────────────────────────────────────┐
│                        Presentation                          │
│          WinUI 3 XAML  ·  WebView2  ·  MainViewModel        │
└────────────────────────────┬─────────────────────────────────┘
                             │  uses
┌────────────────────────────▼─────────────────────────────────┐
│                        Application                           │
│          OpenFileUseCase  ·  RenderDocumentUseCase           │
│          ThemeService                                        │
└──────────┬──────────────────────────────────────┬────────────┘
           │  depends on (interface only)          │
┌──────────▼──────────────┐          ┌─────────────▼───────────┐
│         Domain          │          │      Infrastructure      │
│  Entities  ·  Ports     │◄─────────│  FileRepository          │
│  (Document)             │  impls   │  CmarkParser             │
└─────────────────────────┘          └─────────────────────────┘
```

**의존성 방향 규칙:** 안쪽 레이어는 바깥 레이어를 절대 참조하지 않는다.
Infrastructure → Domain(Port 구현), Presentation → Application → Domain.

---

## 2. 레이어별 책임

### 2.1 Domain Layer

외부 의존성이 전혀 없는 순수 C++ 코드. STL 외 어떠한 라이브러리도 `#include` 하지 않는다.

#### `Document` Entity

```
Document
├── filePath       : std::filesystem::path   — 열린 파일의 절대 경로
├── rawMarkdown    : std::wstring            — 원본 마크다운 텍스트
├── renderedHtml   : std::wstring            — 파싱된 HTML (body 부분)
├── lastModified   : file_time_type          — 마지막 수정 시각 (자동 재로드 판단용)
├── IsLoaded()     : bool                    — 문서가 유효한지 여부
└── Title()        : std::wstring            — 파일명(확장자 제외)을 문서 제목으로 반환
```

#### Ports (인터페이스)

| 인터페이스 | 메서드 | 구현체 |
|---|---|---|
| `IFileRepository` | `ReadAllText`, `LoadAssetText`, `WatchFile`, `StopWatching` | `Infrastructure::FileRepository` |
| `IMarkdownParser` | `ToHtml(wstring) → wstring` | `Infrastructure::CmarkParser` |

---

### 2.2 Application Layer

도메인 포트를 조합해 유스케이스를 구현한다. UI나 OS API를 직접 참조하지 않는다.

#### `OpenFileUseCase`

```
Execute(path) → Document
  1. IFileRepository::ReadAllText(path)  → rawMarkdown
  2. IMarkdownParser::ToHtml(rawMarkdown) → renderedHtml
  3. Document { filePath, rawMarkdown, renderedHtml, lastModified } 반환
```

#### `RenderDocumentUseCase`

```
Execute(Document) → std::wstring (완성된 HTML 페이지)
  1. IFileRepository::LoadAssetText("render-template.html") → template
  2. template 의 {{CONTENT}} 플레이스홀더를 Document.renderedHtml 로 치환
  3. 완성된 HTML 문자열 반환

ExecuteRaw(markdown) → std::wstring
  1. IMarkdownParser::ToHtml(markdown)
  2. Execute() 와 동일한 템플릿 주입 과정
```

#### `ThemeService`

```
AppTheme { Light, Dark, System }

IsDark() → bool
  - System 일 때: HKCU\...\Personalize::AppsUseLightTheme 레지스트리 값으로 판단
  - Light / Dark 일 때: 명시적 설정 값 반환

WebViewThemeScript() → wstring
  - "document.documentElement.setAttribute('data-theme', 'dark');" 형태의 JS 반환
  - C++ → WebView2 ExecuteScriptAsync() 에 직접 전달

OnThemeChanged : std::function<void(AppTheme)>
  - 테마 변경 시 Presentation 레이어에 통지 (옵저버 패턴)
```

---

### 2.3 Infrastructure Layer

외부 라이브러리(cmark-gfm, Win32 API)와의 어댑터 역할.
Domain Port 인터페이스를 구현하므로 Domain은 이 레이어의 존재를 모른다.

#### `FileRepository`

```
ReadAllText(path)
  - std::ifstream (binary mode) → std::ostringstream
  - MultiByteToWideChar(CP_UTF8) 로 UTF-8 → wstring 변환
  - BOM(EF BB BF) 자동 제거

LoadAssetText(assetName)
  - GetModuleFileNameW() 로 실행 파일 경로 취득
  - 실행 파일 옆 Assets\ 폴더에서 파일 로드
  - ReadAllText() 재사용

WatchFile(path, callback)
  - std::thread + std::atomic<bool> m_watching
  - 500ms 간격으로 last_write_time 폴링
  - 변경 감지 시 callback() 호출 (백그라운드 스레드에서 호출됨)
  - 콜백 내부에서 UI 업데이트는 반드시 DispatcherQueue 로 마샬링 필요

StopWatching()
  - m_watching = false → thread join()
```

#### `CmarkParser`

```
ToHtml(wstring markdown)
  1. WideCharToMultiByte(CP_UTF8) 로 wstring → UTF-8 string
  2. cmark_gfm_core_extensions_ensure_registered()
  3. cmark_parser_new(CMARK_OPT_UNSAFE | CMARK_OPT_SMART)
  4. 확장 attach:
     - table         (GFM 파이프 테이블)
     - strikethrough (~~텍스트~~)
     - autolink      (URL 자동 링크화)
     - tasklist      (- [x] 체크박스)
  5. cmark_parser_feed() → cmark_parser_finish()
  6. cmark_render_html() → char*
  7. MultiByteToWideChar 로 wstring 반환
  8. free(html), cmark_node_free, cmark_parser_free (RAII 래퍼로 개선 예정)
```

---

### 2.4 Presentation Layer

WinUI 3 XAML + C++/WinRT 코드비하인드. MVVM 패턴 적용.

#### `MainViewModel`

UI 상태와 Application 유스케이스 사이의 중개자.

```
상태:
  m_currentDoc    : Domain::Document   — 현재 열린 문서
  m_recentFiles   : vector<path>       — 최근 파일 목록 (최대 10개)

공개 커맨드:
  OpenFile(path)  → bool              — 파일 열기 (백그라운드 스레드에서 호출 가능)
  Reload()                            — 현재 파일 재로드
  ToggleTheme()                       — Light ↔ Dark 토글

이벤트 (std::function 콜백):
  OnRenderReady(wstring html)         — 렌더링 완료 시 Presentation에 통지
  OnThemeChanged(bool isDark)         — 테마 변경 시 WebView2 동기화
```

#### `MainWindow` (코드비하인드)

```
InitWebView()
  - EnsureCoreWebView2Async() co_await
  - WebView2.WebMessageReceived 이벤트 구독 (외부 링크 오픈 처리)

LoadDocument(path)                    [fire_and_forget]
  - resume_background() — 파일 I/O + 파싱 (UI 블로킹 방지)
  - ViewModel.OpenFile(path)
  - resume_foreground() — UI 업데이트

NavigateWebView(html)                 [fire_and_forget]
  - CoreWebView2.NavigateToString(html)
  - DropOverlay 숨김, WebView2 표시

ApplyThemeToWebView()                 [fire_and_forget]
  - ExecuteScriptAsync(ThemeService.WebViewThemeScript())
  - hljs 테마 CSS 교체 스크립트 실행
```

---

## 3. 스레딩 모델

```
[UI Thread]
  MainWindow 생성, XAML 이벤트 처리, WebView2 조작
  ↑ resume_foreground(DispatcherQueue)
  ↓ resume_background()

[Thread Pool]
  파일 읽기 (FileRepository::ReadAllText)
  cmark-gfm 파싱 (CmarkParser::ToHtml)

[Watch Thread] (std::thread)
  FileRepository::WatchFile — 500ms 폴링
  콜백 → DispatcherQueue::TryEnqueue() 로 UI Thread에 마샬링
```

**규칙:** WebView2, XAML 컨트롤에 대한 모든 접근은 반드시 UI 스레드에서 수행한다.

---

## 4. 데이터 흐름 요약

```
[사용자: 파일 드롭 / 열기]
       │
       ▼
MainWindow::OnDrop / OnOpenFileClick
       │  co_await resume_background()
       ▼
OpenFileUseCase::Execute(path)
  ├── FileRepository::ReadAllText(path) → rawMarkdown
  └── CmarkParser::ToHtml(rawMarkdown)  → renderedHtml
       │  Document 반환
       ▼
RenderDocumentUseCase::Execute(doc)
  └── template 에 renderedHtml 주입 → fullHtml
       │  co_await resume_foreground()
       ▼
MainWindow::NavigateWebView(fullHtml)
  └── CoreWebView2::NavigateToString(fullHtml)
       │
       ▼
[WebView2: GitHub-style MD 렌더링 완료]
```
