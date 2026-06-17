// MDView/src/Domain/Entities/Document.h
#pragma once
#include <string>
#include <filesystem>
#include <chrono>

namespace MDView::Domain {

    struct Document {
        std::filesystem::path   filePath;
        std::wstring            rawMarkdown;
        std::wstring            renderedHtml;
        std::filesystem::file_time_type lastModified;

        bool IsLoaded() const noexcept {
            return !rawMarkdown.empty();
        }

        std::wstring Title() const {
            if (filePath.empty()) return L"Untitled";
            return filePath.stem().wstring();
        }
    };
}
