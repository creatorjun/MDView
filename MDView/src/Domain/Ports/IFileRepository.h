// MDView/src/Domain/Ports/IFileRepository.h
#pragma once
#include <string>
#include <filesystem>
#include <functional>

namespace MDView::Domain {

    class IFileRepository {
    public:
        virtual ~IFileRepository() = default;

        virtual std::wstring ReadAllText(const std::filesystem::path& path) const = 0;
        virtual std::wstring LoadAssetText(std::wstring_view assetName) const = 0;

        virtual void WatchFile(
            const std::filesystem::path& path,
            std::function<void()> onChanged
        ) = 0;

        virtual void StopWatching() = 0;
    };
}
