// MDView/src/Application/Services/ThemeService.h
#pragma once
#include <string>
#include <functional>

namespace MDView::Application {

    enum class AppTheme { Light, Dark, System };

    class ThemeService {
    public:
        ThemeService();

        AppTheme Current() const noexcept;
        bool     IsDark()  const noexcept;

        void SetTheme(AppTheme theme);

        std::wstring WebViewThemeScript() const;

        std::function<void(AppTheme)> OnThemeChanged;

    private:
        AppTheme m_theme{ AppTheme::System };
        bool     m_systemIsDark{ false };

        bool ResolveSystemDark() const;
    };
}
