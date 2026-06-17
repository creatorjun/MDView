// MDView/src/Application/Services/ThemeService.cpp
#include "../../pch.h"
#include "ThemeService.h"
#include <windows.h>

namespace MDView::Application {

    ThemeService::ThemeService() {
        m_systemIsDark = ResolveSystemDark();
    }

    AppTheme ThemeService::Current() const noexcept {
        return m_theme;
    }

    bool ThemeService::IsDark() const noexcept {
        if (m_theme == AppTheme::Dark)   return true;
        if (m_theme == AppTheme::Light)  return false;
        return m_systemIsDark;
    }

    void ThemeService::SetTheme(AppTheme theme) {
        m_theme = theme;
        if (OnThemeChanged) OnThemeChanged(theme);
    }

    std::wstring ThemeService::WebViewThemeScript() const {
        auto isDark = IsDark();
        std::wstring script =
            L"document.documentElement.setAttribute('data-theme', '";
        script += isDark ? L"dark" : L"light";
        script += L"');";
        return script;
    }

    bool ThemeService::ResolveSystemDark() const {
        DWORD value = 0;
        DWORD size  = sizeof(value);
        LONG  result = RegGetValueW(
            HKEY_CURRENT_USER,
            L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
            L"AppsUseLightTheme",
            RRF_RT_REG_DWORD,
            nullptr,
            &value,
            &size
        );
        if (result == ERROR_SUCCESS) {
            return (value == 0);
        }
        return false;
    }
}
