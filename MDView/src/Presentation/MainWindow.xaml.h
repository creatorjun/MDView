// MDView/src/Presentation/MainWindow.xaml.h
#pragma once
#include "../pch.h"
#include "ViewModels/MainViewModel.h"
#include <memory>

namespace winrt::MDView::Presentation::implementation {

    struct MainWindow : MainWindowT<MainWindow> {
        MainWindow();

        void OnOpenFileClick(IInspectable const&, Microsoft::UI::Xaml::RoutedEventArgs const&);
        void OnReloadClick  (IInspectable const&, Microsoft::UI::Xaml::RoutedEventArgs const&);
        void OnThemeToggleClick(IInspectable const&, Microsoft::UI::Xaml::RoutedEventArgs const&);
        void OnDragOver     (IInspectable const&, Microsoft::UI::Xaml::DragEventArgs const&);
        void OnDrop         (IInspectable const&, Microsoft::UI::Xaml::DragEventArgs const&);

    private:
        winrt::fire_and_forget InitWebView();
        winrt::fire_and_forget LoadDocument(std::filesystem::path path);
        winrt::fire_and_forget NavigateWebView(std::wstring html);
        winrt::fire_and_forget ApplyThemeToWebView();
        winrt::fire_and_forget PickAndOpenFile();

        void UpdateTitleBar();
        void SetDropOverlayVisible(bool visible);

        std::unique_ptr<MDView::Presentation::MainViewModel> m_viewModel;
        bool m_webViewReady{ false };
    };
}

namespace winrt::MDView::Presentation::factory_implementation {
    struct MainWindow : MainWindowT<MainWindow, implementation::MainWindow> {};
}
