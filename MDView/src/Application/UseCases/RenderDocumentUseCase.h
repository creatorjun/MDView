// MDView/src/Application/UseCases/RenderDocumentUseCase.h
#pragma once
#include "../../Domain/Entities/Document.h"
#include "../../Domain/Ports/IFileRepository.h"
#include "../../Domain/Ports/IMarkdownParser.h"
#include <memory>
#include <string>

namespace MDView::Application {

    class RenderDocumentUseCase {
    public:
        RenderDocumentUseCase(
            std::shared_ptr<Domain::IFileRepository> fileRepo,
            std::shared_ptr<Domain::IMarkdownParser>  parser
        );

        std::wstring Execute(const Domain::Document& doc) const;
        std::wstring ExecuteRaw(const std::wstring& markdown) const;

    private:
        std::wstring InjectIntoTemplate(
            const std::wstring& htmlBody,
            const std::wstring& templateContent
        ) const;

        std::shared_ptr<Domain::IFileRepository> m_fileRepo;
        std::shared_ptr<Domain::IMarkdownParser>  m_parser;
    };
}
