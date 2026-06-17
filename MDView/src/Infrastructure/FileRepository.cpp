// MDView/src/Infrastructure/FileRepository.cpp
#include "../pch.h"
#include "FileRepository.h"
#include <fstream>
#include <sstream>

namespace MDView::Infrastructure {

    FileRepository::FileRepository() = default;

    FileRepository::~FileRepository() {
        StopWatching();
    }

    std::wstring FileRepository::ReadAllText(const std::filesystem::path& path) const {
        std::ifstream file(path, std::ios::binary);
        if (!file.is_open()) {
            throw std::runtime_error("Cannot open file: " + path.string());
        }
        std::ostringstream ss;
        ss << file.rdbuf();
        return Utf8ToWide(ss.str());
    }

    std::wstring FileRepository::LoadAssetText(std::wstring_view assetName) const {
        auto assetPath = AssetDirectory() / assetName;
        return ReadAllText(assetPath);
    }

    void FileRepository::WatchFile(
        const std::filesystem::path& path,
        std::function<void()> onChanged)
    {
        StopWatching();
        m_watchedPath = path;
        m_watching.store(true);

        m_watchThread = std::thread([this, path, cb = std::move(onChanged)]() {
            auto lastTime = std::filesystem::last_write_time(path);
            while (m_watching.load()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                if (!std::filesystem::exists(path)) continue;
                auto current = std::filesystem::last_write_time(path);
                if (current != lastTime) {
                    lastTime = current;
                    if (cb) cb();
                }
            }
        });
    }

    void FileRepository::StopWatching() {
        m_watching.store(false);
        if (m_watchThread.joinable()) {
            m_watchThread.join();
        }
    }

    std::wstring FileRepository::Utf8ToWide(std::string_view utf8) {
        if (utf8.empty()) return {};
        int len = MultiByteToWideChar(
            CP_UTF8, 0,
            utf8.data(), static_cast<int>(utf8.size()),
            nullptr, 0
        );
        std::wstring result(len, L'\0');
        MultiByteToWideChar(
            CP_UTF8, 0,
            utf8.data(), static_cast<int>(utf8.size()),
            result.data(), len
        );
        return result;
    }

    std::filesystem::path FileRepository::AssetDirectory() {
        wchar_t exePath[MAX_PATH]{};
        GetModuleFileNameW(nullptr, exePath, MAX_PATH);
        return std::filesystem::path(exePath).parent_path() / L"Assets";
    }
}
