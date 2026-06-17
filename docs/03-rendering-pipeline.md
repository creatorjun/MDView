# 렌더링 파이프라인

## 1. 전체 흐름

```
[.md 파일]
    │
    │  FileRepository::ReadAllText()
    │  UTF-8 → std::wstring
    ▼
[raw Markdown wstring]
    │
    │  CmarkParser::ToHtml()
    │  cmark-gfm (GFM 확장 포함)
    ▼
[HTML body fragment]
    │
    │  RenderDocumentUseCase::InjectIntoTemplate()
    │  {{CONTENT}} 치환
    ▼
[완성된 HTML 페이지 (wstring)]
    │
    │  CoreWebView2::NavigateToString()
    ▼
[WebView2 렌더링]
    ├── Pretendard 폰트 로드
    ├── CSS 토큰 적용 (Light / Dark)
    ├── highlight.js 코드 하이라이팅
    └── KaTeX 수식 렌더링
```

---

## 2. cmark-gfm 파서 상세

### 활성화된 GFM 확장

| 확장 이름 | 기능 | 예시 |
|---|---|---|
| `table` | 파이프 테이블 | `\| A \| B \|` |
| `strikethrough` | 취소선 | `~~텍스트~~` |
| `autolink` | URL 자동 링크 | `https://example.com` |
| `tasklist` | 체크박스 목록 | `- [x] 완료` |

### 파서 옵션

```cpp
cmark_parser_new(
    CMARK_OPT_UNSAFE  // raw HTML 허용 (HTML 블록 렌더링)
  | CMARK_OPT_SMART   // 스마트 따옴표, 대시 변환
)
```

`CMARK_OPT_UNSAFE` 는 `<iframe>`, `<script>` 등 raw HTML 블록을 렌더링한다.
WebView2 의 보안 샌드박스가 악성 스크립트를 차단하므로 뷰어 용도에서는 허용한다.
필요 시 `CMARK_OPT_SAFE` 로 교체하면 raw HTML을 주석으로 대체한다.

### RAII 래퍼 (M3 구현 예정)

현재 `free()` / `cmark_node_free()` / `cmark_parser_free()` 를 수동 호출한다.
예외 안전성을 위해 다음 RAII 래퍼를 추가할 예정이다:

```cpp
struct CmarkParserDeleter {
    void operator()(cmark_parser* p) const { cmark_parser_free(p); }
};
using CmarkParserPtr = std::unique_ptr<cmark_parser, CmarkParserDeleter>;

struct CmarkNodeDeleter {
    void operator()(cmark_node* n) const { cmark_node_free(n); }
};
using CmarkNodePtr = std::unique_ptr<cmark_node, CmarkNodeDeleter>;
```

---

## 3. render-template.html 구조

```html
<!DOCTYPE html>
<html data-theme="light">        ← C++ ExecuteScriptAsync 로 "dark" 전환
<head>
  <!-- Pretendard (한글+영문 최적화) -->
  <!-- highlight.js (syntax highlighting) -->
  <!-- KaTeX (수식 렌더링) -->
  <style>                         ← 전체 CSS 인라인 (CDN 의존 없음 목표)
    :root { ... }                 ← Light 토큰
    [data-theme="dark"] { ... }   ← Dark 토큰
    /* typography, code, table, blockquote ... */
  </style>
</head>
<body>
  <article id="md-content">
    {{CONTENT}}                   ← C++ 에서 치환되는 플레이스홀더
  </article>
  <script>
    hljs.highlightAll()           ← 코드블록 하이라이팅
    window.setTheme(theme)        ← C++ 에서 호출 가능한 전역 함수
    // 앵커 링크 생성
    // 외부 링크 인터셉트 → postMessage
  </script>
</body>
</html>
```

### CSS 디자인 토큰

#### Light 모드 (GitHub 팔레트 기반)

| 토큰 | 값 | 용도 |
|---|---|---|
| `--color-bg` | `#ffffff` | 페이지 배경 |
| `--color-surface` | `#f6f8fa` | 코드블록, 테이블 헤더 |
| `--color-border` | `#d0d7de` | 구분선, 테이블 외곽 |
| `--color-text` | `#1f2328` | 본문 텍스트 |
| `--color-text-muted` | `#636c76` | 보조 텍스트, blockquote |
| `--color-primary` | `#0969da` | 링크, 강조 |
| `--font-body` | Pretendard Variable | 본문, 제목 |
| `--font-mono` | Cascadia Code / Fira Code | 코드블록 |

#### Dark 모드 (GitHub Dark 팔레트 기반)

| 토큰 | 값 | 용도 |
|---|---|---|
| `--color-bg` | `#0d1117` | 페이지 배경 |
| `--color-surface` | `#161b22` | 코드블록, 테이블 헤더 |
| `--color-border` | `#30363d` | 구분선 |
| `--color-text` | `#e6edf3` | 본문 텍스트 |
| `--color-primary` | `#58a6ff` | 링크, 강조 |

---

## 4. C++ ↔ JavaScript 통신

### C++ → WebView2 (단방향 명령)

`ExecuteScriptAsync()` 로 JavaScript 코드를 직접 실행한다.

```cpp
// 테마 전환
co_await webView.CoreWebView2().ExecuteScriptAsync(
    L"window.setTheme('dark');"
);

// 스크롤 위치 조회 (반환값 활용)
auto result = co_await webView.CoreWebView2().ExecuteScriptAsync(
    L"JSON.stringify(window.scrollY)"
);
int scrollY = std::stoi(std::wstring(result));

// 스크롤 위치 복원
co_await webView.CoreWebView2().ExecuteScriptAsync(
    std::format(L"window.scrollTo(0, {});", scrollY)
);
```

### WebView2 → C++ (이벤트 기반)

JS 에서 `window.chrome.webview.postMessage(data)` 로 메시지를 전송하면
C++ 의 `WebMessageReceived` 이벤트 핸들러가 수신한다.

```cpp
// C++ 구독
coreWebView2.WebMessageReceived(
    [this](CoreWebView2 const&, CoreWebView2WebMessageReceivedEventArgs const& args) {
        auto json = args.WebMessageAsJson();
        // JSON 파싱 → type 에 따라 분기
        // "openUrl" → ShellExecuteW 로 기본 브라우저 오픈
    }
);
```

```javascript
// JS 전송 (render-template.html)
document.querySelectorAll('a[href^="http"]').forEach(a => {
    a.addEventListener('click', e => {
        e.preventDefault();
        window.chrome.webview.postMessage({
            type: 'openUrl',
            url: a.href
        });
    });
});
```

### NavigationCompleted 이벤트 활용

`NavigateToString()` 완료 후 실행이 필요한 초기화 작업(스크롤 복원, 테마 적용)은
`NavigationCompleted` 이벤트에서 처리한다.

```cpp
coreWebView2.NavigationCompleted(
    [this](CoreWebView2 const&, CoreWebView2NavigationCompletedEventArgs const& args) {
        if (args.IsSuccess()) {
            ApplyThemeToWebView();
            RestoreScrollPosition();
        }
    }
);
```

---

## 5. 오프라인 에셋 매핑 (M3)

CDN 의존성을 제거하기 위해 WebView2 가상 호스트 매핑을 사용한다.

```cpp
// InitWebView() 에서 한 번 설정
coreWebView2.SetVirtualHostNameToFolderMapping(
    L"mdview.assets",                      // 가상 호스트명
    assetFolderPath.wstring(),             // 실제 폴더 경로
    CoreWebView2HostResourceAccessKind::Allow
);
```

`render-template.html` 에서 CDN URL 대신:

```html
<script src="https://mdview.assets/highlight.min.js"></script>
<link  href="https://mdview.assets/pretendard.css" rel="stylesheet">
```

`Assets/` 폴더에 번들할 파일:

```
MDView/Assets/
├── render-template.html
├── highlight.min.js
├── styles/
│   ├── github.min.css
│   └── github-dark.min.css
├── katex.min.js
├── auto-render.min.js
├── katex.min.css
└── fonts/
    └── pretendard-variable.woff2
```
