// MDView/src/App.h
#pragma once
#include "pch.h"

namespace winrt::MDView::implementation {

    struct App : AppT<App> {
        App();
        void OnLaunched(Microsoft::UI::Xaml::LaunchActivatedEventArgs const&);

    private:
        winrt::Microsoft::UI::Xaml::Window m_window{ nullptr };
    };
}

namespace winrt::MDView::factory_implementation {
    struct App : AppT<App, implementation::App> {};
}
