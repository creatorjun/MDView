// MDView/src/Presentation/ViewModels/MainViewModel.cpp
#include "../../pch.h"
#include "MainViewModel.h"
#include <algorithm>

namespace MDView::Presentation {

    MainViewModel::MainViewModel(
        std::shared_ptr<Application::OpenFileUseCase>    openFile,
        std::shared_ptr<Application::RenderDocumentUseCase> render,
        std::shared_ptr<Application::ThemeService>       theme)
        : m_openFile(std::move(openFile))
        , m_render(std::move(render))
        , m_theme(std::move(theme))
    {
        m_theme->OnThemeChanged = [this](Application::AppTheme) {
            if (OnThemeChanged) OnThemeChanged(m_theme->IsDark());
            if (HasDocument()) NotifyRenderReady();
        };
    }

    bool MainViewModel::OpenFile(const std::filesystem::path& path) {
        try {
            m_currentDoc = m_openFile->Execute(path);
            PushRecentFile(path);
            NotifyRenderReady();
            return true;
        }
        catch (...) {
            return false;
        }
    }

    void MainViewModel::Reload() {
        if (!HasDocument()) return;
        OpenFile(m_currentDoc.filePath);
    }

    void MainViewModel::ToggleTheme() {
        auto next = m_theme->IsDark()
            ? Application::AppTheme::Light
            : Application::AppTheme::Dark;
        m_theme->SetTheme(next);
    }

    const Domain::Document& MainViewModel::CurrentDocument() const noexcept {
        return m_currentDoc;
    }

    std::wstring MainViewModel::RenderedHtml() const {
        if (!HasDocument()) return {};
        return m_render->Execute(m_currentDoc);
    }

    bool MainViewModel::HasDocument() const noexcept {
        return m_currentDoc.IsLoaded();
    }

    std::wstring MainViewModel::WindowTitle() const {
        if (!HasDocument()) return L"MDView";
        return m_currentDoc.Title() + L" — MDView";
    }

    bool MainViewModel::IsDark() const noexcept {
        return m_theme->IsDark();
    }

    const std::vector<std::filesystem::path>& MainViewModel::RecentFiles() const noexcept {
        return m_recentFiles;
    }

    void MainViewModel::NotifyRenderReady() {
        if (OnRenderReady) OnRenderReady(RenderedHtml());
    }

    void MainViewModel::PushRecentFile(const std::filesystem::path& path) {
        auto it = std::find(m_recentFiles.begin(), m_recentFiles.end(), path);
        if (it != m_recentFiles.end()) m_recentFiles.erase(it);
        m_recentFiles.insert(m_recentFiles.begin(), path);
        if (m_recentFiles.size() > kMaxRecentFiles) {
            m_recentFiles.resize(kMaxRecentFiles);
        }
    }
}
