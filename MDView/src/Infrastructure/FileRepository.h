// MDView/src/Infrastructure/FileRepository.h
#pragma once
#include "../Domain/Ports/IFileRepository.h"
#include <atomic>
#include <thread>

namespace MDView::Infrastructure {

    class FileRepository final : public Domain::IFileRepository {
    public:
        FileRepository();
        ~FileRepository() override;

        std::wstring ReadAllText(const std::filesystem::path& path) const override;
        std::wstring LoadAssetText(std::wstring_view assetName) const override;

        void WatchFile(
            const std::filesystem::path& path,
            std::function<void()> onChanged
        ) override;

        void StopWatching() override;

    private:
        std::thread             m_watchThread;
        std::atomic<bool>       m_watching{ false };
        std::filesystem::path   m_watchedPath;

        static std::wstring Utf8ToWide(std::string_view utf8);
        static std::filesystem::path AssetDirectory();
    };
}
