# MDView

A beautiful Markdown viewer for Windows, built with **WinUI 3** (C++/WinRT) + **WebView2**.

## Architecture

Clean Architecture (Hexagonal) with four layers:

```
Presentation (WinUI 3 XAML + WebView2)
    └── Application (Use Cases)
            └── Domain (Entities + Ports)
    Infrastructure (cmark-gfm parser + Win32 file I/O)
```

## Features

- GitHub-flavored Markdown rendering (GFM)
- Light / Dark theme with system sync
- Drag & Drop file open
- Recent files list
- Syntax-highlighted code blocks (highlight.js)
- Live file watcher (auto-reload on save)
- Beautiful typography (Pretendard font)

## Requirements

- Windows 10 22H2 or later (Windows 11 recommended)
- Visual Studio 2022 17.8+
  - Workload: **Windows App SDK** (C++ tools)
- vcpkg (manifest mode)
- WebView2 Runtime (ships with Windows 11; auto-installed on Win10)

## Build

```powershell
# 1. Install vcpkg dependencies
vcpkg install

# 2. Open solution
start MDView.sln

# 3. Build (x64 | Debug)
```

## Dependencies

| Package | Purpose |
|---|---|
| `Microsoft.WindowsAppSDK` | WinUI 3 runtime |
| `Microsoft.Web.WebView2` | Chromium-based renderer |
| `cmark-gfm` (vcpkg) | Markdown → HTML (GFM) |

## Rendering Stack

Markdown files are parsed to HTML by **cmark-gfm** in the Infrastructure layer,
then injected into a local HTML template loaded inside **WebView2**.
The template ships with:

- Custom CSS (GitHub-inspired, dark-mode aware)
- `highlight.js` for syntax highlighting
- `KaTeX` for math rendering
- Smooth scroll + anchor navigation
