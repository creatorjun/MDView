// MDView/src/App.cpp
#include "pch.h"
#include "App.h"
#include "Presentation/MainWindow.xaml.h"

namespace winrt::MDView::implementation {

    App::App() {
        InitializeComponent();
#if defined _DEBUG && !defined DISABLE_XAML_GENERATED_BREAK_ON_UNHANDLED_EXCEPTION
        UnhandledException([](IInspectable const&, Microsoft::UI::Xaml::UnhandledExceptionEventArgs const& e) {
            if (IsDebuggerPresent()) {
                auto errorMessage = e.Message();
                __debugbreak();
            }
        });
#endif
    }

    void App::OnLaunched(Microsoft::UI::Xaml::LaunchActivatedEventArgs const&) {
        m_window = make<Presentation::implementation::MainWindow>();
        m_window.Activate();
    }
}
