// MDView/src/Presentation/ViewModels/MainViewModel.h
#pragma once
#include "../../pch.h"
#include "../../Domain/Entities/Document.h"
#include "../../Application/UseCases/OpenFileUseCase.h"
#include "../../Application/UseCases/RenderDocumentUseCase.h"
#include "../../Application/Services/ThemeService.h"
#include <memory>
#include <vector>
#include <filesystem>

namespace MDView::Presentation {

    class MainViewModel {
    public:
        MainViewModel(
            std::shared_ptr<Application::OpenFileUseCase>    openFile,
            std::shared_ptr<Application::RenderDocumentUseCase> render,
            std::shared_ptr<Application::ThemeService>       theme
        );

        bool OpenFile(const std::filesystem::path& path);
        void Reload();
        void ToggleTheme();

        const Domain::Document& CurrentDocument() const noexcept;
        std::wstring            RenderedHtml()     const;
        bool                    HasDocument()      const noexcept;
        std::wstring            WindowTitle()      const;
        bool                    IsDark()           const noexcept;

        std::function<void(const std::wstring&)> OnRenderReady;
        std::function<void(bool)>                OnThemeChanged;

        const std::vector<std::filesystem::path>& RecentFiles() const noexcept;

    private:
        void NotifyRenderReady();
        void PushRecentFile(const std::filesystem::path& path);

        std::shared_ptr<Application::OpenFileUseCase>       m_openFile;
        std::shared_ptr<Application::RenderDocumentUseCase> m_render;
        std::shared_ptr<Application::ThemeService>          m_theme;

        Domain::Document                    m_currentDoc;
        std::vector<std::filesystem::path>  m_recentFiles;

        static constexpr size_t kMaxRecentFiles = 10;
    };
}
