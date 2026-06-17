// MDView/src/Infrastructure/CmarkParser.cpp
#include "../pch.h"
#include "CmarkParser.h"
#include <cmark-gfm.h>
#include <cmark-gfm-extension_api.h>
#include <table.h>
#include <strikethrough.h>
#include <autolink.h>
#include <tasklist.h>

namespace MDView::Infrastructure {

    std::wstring CmarkParser::ToHtml(const std::wstring& markdown) const {
        auto utf8 = WideToUtf8(markdown);

        cmark_gfm_core_extensions_ensure_registered();

        cmark_parser* parser = cmark_parser_new(
            CMARK_OPT_UNSAFE |
            CMARK_OPT_SMART
        );

        cmark_syntax_extension* tableExt       = cmark_find_syntax_extension("table");
        cmark_syntax_extension* strikethroughExt = cmark_find_syntax_extension("strikethrough");
        cmark_syntax_extension* autolinkExt    = cmark_find_syntax_extension("autolink");
        cmark_syntax_extension* tasklistExt    = cmark_find_syntax_extension("tasklist");

        if (tableExt)        cmark_parser_attach_syntax_extension(parser, tableExt);
        if (strikethroughExt)cmark_parser_attach_syntax_extension(parser, strikethroughExt);
        if (autolinkExt)     cmark_parser_attach_syntax_extension(parser, autolinkExt);
        if (tasklistExt)     cmark_parser_attach_syntax_extension(parser, tasklistExt);

        cmark_parser_feed(parser, utf8.c_str(), utf8.size());
        cmark_node* doc = cmark_parser_finish(parser);

        char* html = cmark_render_html(
            doc,
            CMARK_OPT_UNSAFE | CMARK_OPT_SMART,
            nullptr
        );

        auto result = Utf8ToWide(html);

        free(html);
        cmark_node_free(doc);
        cmark_parser_free(parser);

        return result;
    }

    std::string CmarkParser::WideToUtf8(const std::wstring& wide) {
        if (wide.empty()) return {};
        int len = WideCharToMultiByte(
            CP_UTF8, 0,
            wide.data(), static_cast<int>(wide.size()),
            nullptr, 0, nullptr, nullptr
        );
        std::string result(len, '\0');
        WideCharToMultiByte(
            CP_UTF8, 0,
            wide.data(), static_cast<int>(wide.size()),
            result.data(), len, nullptr, nullptr
        );
        return result;
    }

    std::wstring CmarkParser::Utf8ToWide(std::string_view utf8) {
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
}
