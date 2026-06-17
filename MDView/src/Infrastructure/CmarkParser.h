// MDView/src/Infrastructure/CmarkParser.h
#pragma once
#include "../Domain/Ports/IMarkdownParser.h"

namespace MDView::Infrastructure {

    class CmarkParser final : public Domain::IMarkdownParser {
    public:
        CmarkParser() = default;
        ~CmarkParser() override = default;

        std::wstring ToHtml(const std::wstring& markdown) const override;

    private:
        static std::string  WideToUtf8(const std::wstring& wide);
        static std::wstring Utf8ToWide(std::string_view utf8);
    };
}
