// MDView/src/Application/UseCases/RenderDocumentUseCase.cpp
#include "../../pch.h"
#include "RenderDocumentUseCase.h"

namespace MDView::Application {

    RenderDocumentUseCase::RenderDocumentUseCase(
        std::shared_ptr<Domain::IFileRepository> fileRepo,
        std::shared_ptr<Domain::IMarkdownParser>  parser)
        : m_fileRepo(std::move(fileRepo))
        , m_parser(std::move(parser))
    {}

    std::wstring RenderDocumentUseCase::Execute(const Domain::Document& doc) const {
        auto tmpl = m_fileRepo->LoadAssetText(L"render-template.html");
        return InjectIntoTemplate(doc.renderedHtml, tmpl);
    }

    std::wstring RenderDocumentUseCase::ExecuteRaw(const std::wstring& markdown) const {
        auto html = m_parser->ToHtml(markdown);
        auto tmpl = m_fileRepo->LoadAssetText(L"render-template.html");
        return InjectIntoTemplate(html, tmpl);
    }

    std::wstring RenderDocumentUseCase::InjectIntoTemplate(
        const std::wstring& htmlBody,
        const std::wstring& templateContent) const
    {
        auto result = templateContent;
        auto pos = result.find(L"{{CONTENT}}");
        if (pos != std::wstring::npos) {
            result.replace(pos, 11, htmlBody);
        }
        return result;
    }
}
