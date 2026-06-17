# 개발 가이드

## 1. 개발 환경 요구사항

| 항목 | 버전 | 비고 |
|---|---|---|
| Windows | 10 22H2 (19045) 이상 | Windows 11 권장 |
| Visual Studio | 2022 17.8+ | Community 이상 |
| VS 워크로드 | Windows App SDK (C++ 도구) | 필수 |
| vcpkg | 최신 | manifest 모드 사용 |
| WebView2 Runtime | 자동 설치 | Win11 내장, Win10 자동 다운로드 |

### Visual Studio 워크로드 설치

```
Visual Studio Installer → 수정 →
  [✓] Windows 애플리케이션 개발
      [✓] C++ (v143) 유니버설 Windows 플랫폼 도구
      [✓] Windows 앱 SDK C++ 템플릿
```

---

## 2. 빌드 방법

### 최초 세팅

```powershell
# 1. 레포지토리 클론
git clone https://github.com/creatorjun/MDView.git
cd MDView

# 2. vcpkg 설치 (없는 경우)
git clone https://github.com/microsoft/vcpkg.git C:\vcpkg
C:\vcpkg\bootstrap-vcpkg.bat

# 3. vcpkg 환경변수 설정 (PowerShell 프로파일에 추가 권장)
$env:VCPKG_ROOT = "C:\vcpkg"

# 4. 의존성 설치 (vcpkg.json 기반 manifest 모드)
cd MDView
vcpkg install
# → cmark-gfm 이 MDView\vcpkg_installed\x64-windows\ 에 설치됨
```

### Visual Studio에서 빌드

```
1. MDView.sln 더블클릭
2. NuGet 패키지 자동 복원 (최초 1회)
   - Microsoft.WindowsAppSDK 1.5.x
   - Microsoft.Web.WebView2 1.0.x
3. 플랫폼: x64 | 구성: Debug
4. F7 빌드
```

### 커맨드라인 빌드

```powershell
# Developer PowerShell for VS 2022 에서 실행
msbuild MDView.sln `
    /p:Configuration=Debug `
    /p:Platform=x64 `
    /m  # 병렬 빌드
```

---

## 3. 프로젝트 설정 핵심 사항

### C++ 표준 및 컴파일러 옵션

```xml
<!-- MDView.vcxproj -->
<LanguageStandard>stdcpp20</LanguageStandard>
<AdditionalOptions>/await /bigobj %(AdditionalOptions)</AdditionalOptions>
```

- `/await` — C++/WinRT 코루틴 (`co_await`, `co_return`) 지원
- `/bigobj` — XAML 생성 코드의 대용량 오브젝트 파일 처리
- `stdcpp20` — `std::format`, `std::filesystem` 등 C++20 기능 사용

### 출력 디렉토리

```
out\Debug\x64\      ← 실행 파일
out\intermediate\   ← 중간 오브젝트 파일
```

빌드 결과물은 `.gitignore` 에 의해 추적되지 않는다.

---

## 4. 코딩 컨벤션

### 네이밍

| 대상 | 규칙 | 예시 |
|---|---|---|
| 클래스 / 구조체 | PascalCase | `FileRepository`, `MainViewModel` |
| 메서드 / 함수 | PascalCase | `ReadAllText()`, `ToHtml()` |
| 멤버 변수 | `m_` 접두사 + camelCase | `m_currentDoc`, `m_watching` |
| 지역 변수 | camelCase | `rawMarkdown`, `fullHtml` |
| 상수 | `k` 접두사 + PascalCase | `kMaxRecentFiles` |
| 인터페이스 | `I` 접두사 + PascalCase | `IFileRepository` |
| 열거형 값 | PascalCase | `AppTheme::Dark` |

### 파일 헤더

모든 파일 최상단에 경로 주석 **한 줄만** 작성한다. 그 외 주석은 코드 내에 작성하지 않는다.

```cpp
// MDView/src/Infrastructure/CmarkParser.cpp
```

### 헤더 가드

`#pragma once` 를 사용한다. 전통적인 `#ifndef` 가드는 사용하지 않는다.

### 스마트 포인터

| 상황 | 사용 |
|---|---|
| 단독 소유권 | `std::unique_ptr` |
| 공유 소유권 (DI 주입) | `std::shared_ptr` |
| 소유권 없는 참조 | raw pointer 또는 reference |
| COM / WinRT 객체 | `winrt::com_ptr` 또는 WinRT 투영 타입 직접 사용 |

### 비동기 패턴 (C++/WinRT)

```cpp
// UI 스레드 → 백그라운드 전환
winrt::fire_and_forget MainWindow::LoadDocument(std::filesystem::path path) {
    auto lifetime = get_strong();          // 생명주기 연장
    co_await winrt::resume_background();   // Thread Pool

    auto result = m_viewModel->OpenFile(path);  // 파일 I/O + 파싱

    co_await winrt::resume_foreground(DispatcherQueue());  // UI Thread
    // UI 업데이트
}
```

**규칙:**
- `fire_and_forget` 함수는 반드시 첫 줄에 `auto lifetime = get_strong()` 작성
- UI 컨트롤 접근은 항상 `resume_foreground()` 이후
- `EnsureCoreWebView2Async()` 등 WinRT 비동기는 UI 스레드에서 `co_await`

### 에러 처리

- Infrastructure 레이어: `std::runtime_error` 또는 `winrt::hresult_error` throw
- Application 레이어: try/catch 후 `bool` 또는 `std::optional` 반환
- Presentation 레이어: catch 후 `InfoBar` 표시

---

## 5. 레이어별 의존성 규칙

```
허용된 #include 방향:

Presentation  → Application, Domain, Infrastructure (DI 주입만)
Application   → Domain
Infrastructure → Domain (Port 구현), 외부 라이브러리
Domain        → STL 만 허용 (cmark-gfm, Win32, WinRT 불가)
```

위반 예시 (절대 금지):

```cpp
// Domain/Entities/Document.h — WRONG
#include <winrt/Windows.Storage.h>  // WinRT 의존 금지!
#include <cmark-gfm.h>              // 외부 라이브러리 의존 금지!
```

---

## 6. Assets 폴더 관리

`render-template.html` 을 수정할 때는 다음 사항을 확인한다:

- `{{CONTENT}}` 플레이스홀더가 정확히 하나 존재하는지 확인
- `data-theme` 속성이 `<html>` 태그에 있는지 확인
- `window.setTheme(theme)` 전역 함수가 정의되어 있는지 확인
- `window.chrome.webview.postMessage(...)` 외부 링크 인터셉트 코드 유지

빌드 시 `Assets\` 폴더는 실행 파일과 같은 디렉토리에 복사된다.
(`CopyToOutputDirectory = PreserveNewest` 설정)

---

## 7. 자주 발생하는 빌드 오류

### `cmark-gfm.h: No such file or directory`

```powershell
# MDView 디렉토리에서 실행
vcpkg install
# vcpkg-configuration.json 의 baseline SHA가 유효한지 확인
```

### `winrt::hresult_error`: WebView2 초기화 실패

```
원인: WebView2 Runtime 미설치
해결: https://developer.microsoft.com/microsoft-edge/webview2/ 에서 Evergreen Bootstrapper 다운로드 및 실행
```

### XAML 컴파일 오류: `MainWindow.xaml.g.h not found`

```
원인: XAML → C++ 코드 생성 실패
해결: 솔루션 정리(Clean) → 재빌드(Rebuild)
```

### `C2760` 또는 코루틴 관련 오류

```
원인: /await 옵션 누락 또는 C++20 미설정
해결: MDView.vcxproj 의 LanguageStandard = stdcpp20, /await 옵션 확인
```
