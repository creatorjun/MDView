// MDView/src/Domain/Ports/IMarkdownParser.h
#pragma once
#include <string>

namespace MDView::Domain {

    class IMarkdownParser {
    public:
        virtual ~IMarkdownParser() = default;

        virtual std::wstring ToHtml(const std::wstring& markdown) const = 0;
    };
}
