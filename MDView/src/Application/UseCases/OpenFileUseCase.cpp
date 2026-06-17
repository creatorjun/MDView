// MDView/src/Application/UseCases/OpenFileUseCase.cpp
#include "../../pch.h"
#include "OpenFileUseCase.h"

namespace MDView::Application {

    OpenFileUseCase::OpenFileUseCase(
        std::shared_ptr<Domain::IFileRepository> fileRepo,
        std::shared_ptr<Domain::IMarkdownParser>  parser)
        : m_fileRepo(std::move(fileRepo))
        , m_parser(std::move(parser))
    {}

    Domain::Document OpenFileUseCase::Execute(const std::filesystem::path& path) const {
        Domain::Document doc;
        doc.filePath     = path;
        doc.rawMarkdown  = m_fileRepo->ReadAllText(path);
        doc.renderedHtml = m_parser->ToHtml(doc.rawMarkdown);
        doc.lastModified = std::filesystem::last_write_time(path);
        return doc;
    }
}
