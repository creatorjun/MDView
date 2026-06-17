// MDView/src/Presentation/MainWindow.xaml.cpp
#include "../pch.h"
#include "MainWindow.xaml.h"
#include "../Infrastructure/FileRepository.h"
#include "../Infrastructure/CmarkParser.h"
#include "../Application/UseCases/OpenFileUseCase.h"
#include "../Application/UseCases/RenderDocumentUseCase.h"
#include "../Application/Services/ThemeService.h"
#include <winrt/Windows.Storage.Pickers.h>
#include <winrt/Windows.ApplicationModel.DataTransfer.h>

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Windows::Storage::Pickers;
using namespace Windows::ApplicationModel::DataTransfer;

namespace winrt::MDView::Presentation::implementation {

    MainWindow::MainWindow() {
        InitializeComponent();

        auto fileRepo = std::make_shared<MDView::Infrastructure::FileRepository>();
        auto parser   = std::make_shared<MDView::Infrastructure::CmarkParser>();
        auto theme    = std::make_shared<MDView::Application::ThemeService>();

        auto openUC   = std::make_shared<MDView::Application::OpenFileUseCase>(fileRepo, parser);
        auto renderUC = std::make_shared<MDView::Application::RenderDocumentUseCase>(fileRepo, parser);

        m_viewModel = std::make_unique<MDView::Presentation::MainViewModel>(
            openUC, renderUC, theme
        );

        m_viewModel->OnRenderReady = [this](const std::wstring& html) {
            NavigateWebView(html);
            UpdateTitleBar();
        };

        m_viewModel->OnThemeChanged = [this](bool) {
            ApplyThemeToWebView();
        };

        Title(L"MDView");
        InitWebView();
    }

    winrt::fire_and_forget MainWindow::InitWebView() {
        auto lifetime = get_strong();
        co_await MarkdownWebView().EnsureCoreWebView2Async();
        m_webViewReady = true;
    }

    winrt::fire_and_forget MainWindow::PickAndOpenFile() {
        auto lifetime = get_strong();

        FileOpenPicker picker;
        picker.as<IInitializeWithWindow>()->Initialize(
            winrt::Microsoft::UI::GetWindowFromWindowId(
                this->AppWindow().Id()
            )
        );
        picker.ViewMode(PickerViewMode::List);
        picker.SuggestedStartLocation(PickerLocationId::DocumentsLibrary);
        picker.FileTypeFilter().Append(L".md");
        picker.FileTypeFilter().Append(L".markdown");
        picker.FileTypeFilter().Append(L".mdown");

        auto file = co_await picker.PickSingleFileAsync();
        if (!file) co_return;

        co_await LoadDocument(file.Path().c_str());
    }

    winrt::fire_and_forget MainWindow::LoadDocument(std::filesystem::path path) {
        auto lifetime = get_strong();
        co_await winrt::resume_background();
        bool ok = m_viewModel->OpenFile(path);
        co_await winrt::resume_foreground(DispatcherQueue());
        if (ok) {
            SetDropOverlayVisible(false);
            ReloadButton().IsEnabled(true);
        }
    }

    winrt::fire_and_forget MainWindow::NavigateWebView(std::wstring html) {
        auto lifetime = get_strong();
        co_await winrt::resume_foreground(DispatcherQueue());
        if (!m_webViewReady) {
            co_await MarkdownWebView().EnsureCoreWebView2Async();
            m_webViewReady = true;
        }
        MarkdownWebView().Visibility(Visibility::Visible);
        MarkdownWebView().CoreWebView2().NavigateToString(html);
    }

    winrt::fire_and_forget MainWindow::ApplyThemeToWebView() {
        auto lifetime = get_strong();
        co_await winrt::resume_foreground(DispatcherQueue());
        if (!m_webViewReady) co_return;
        auto script = m_viewModel->CurrentDocument().IsLoaded()
            ? std::wstring{}
            : std::wstring{};
        auto themeScript = hstring(
            m_viewModel->IsDark()
                ? L"document.documentElement.setAttribute('data-theme','dark');"
                : L"document.documentElement.setAttribute('data-theme','light');"
        );
        co_await MarkdownWebView().CoreWebView2().ExecuteScriptAsync(themeScript);
    }

    void MainWindow::OnOpenFileClick(
        IInspectable const&, RoutedEventArgs const&) {
        PickAndOpenFile();
    }

    void MainWindow::OnReloadClick(
        IInspectable const&, RoutedEventArgs const&) {
        if (m_viewModel->HasDocument()) {
            LoadDocument(m_viewModel->CurrentDocument().filePath);
        }
    }

    void MainWindow::OnThemeToggleClick(
        IInspectable const&, RoutedEventArgs const&) {
        m_viewModel->ToggleTheme();
        bool dark = m_viewModel->IsDark();
        ThemeIcon().Glyph(dark ? L"\uE793" : L"\uE706");
    }

    void MainWindow::OnDragOver(
        IInspectable const&, DragEventArgs const& e) {
        e.AcceptedOperation(DataPackageOperation::Copy);
        e.DragUIOverride().Caption(L"Open Markdown file");
        e.DragUIOverride().IsGlyphVisible(true);
    }

    void MainWindow::OnDrop(
        IInspectable const&, DragEventArgs const& e) {
        auto items = e.DataView();
        if (items.Contains(StandardDataFormats::StorageItems())) {
            auto op = items.GetStorageItemsAsync();
            op.Completed([this](auto&& result, auto&&) {
                auto files = result.GetResults();
                if (files.Size() > 0) {
                    auto path = files.GetAt(0).Path().c_str();
                    LoadDocument(path);
                }
            });
        }
    }

    void MainWindow::UpdateTitleBar() {
        TitleText().Text(m_viewModel->WindowTitle());
        FilePathText().Text(
            m_viewModel->HasDocument()
                ? m_viewModel->CurrentDocument().filePath.wstring()
                : L""
        );
    }

    void MainWindow::SetDropOverlayVisible(bool visible) {
        DropOverlay().Visibility(
            visible ? Visibility::Visible : Visibility::Collapsed
        );
        MarkdownWebView().Visibility(
            visible ? Visibility::Collapsed : Visibility::Visible
        );
    }
}
