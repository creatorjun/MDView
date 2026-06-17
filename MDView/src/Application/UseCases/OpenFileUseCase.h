// MDView/src/Application/UseCases/OpenFileUseCase.h
#pragma once
#include "../../Domain/Entities/Document.h"
#include "../../Domain/Ports/IFileRepository.h"
#include "../../Domain/Ports/IMarkdownParser.h"
#include <memory>
#include <filesystem>

namespace MDView::Application {

    class OpenFileUseCase {
    public:
        OpenFileUseCase(
            std::shared_ptr<Domain::IFileRepository> fileRepo,
            std::shared_ptr<Domain::IMarkdownParser>  parser
        );

        Domain::Document Execute(const std::filesystem::path& path) const;

    private:
        std::shared_ptr<Domain::IFileRepository> m_fileRepo;
        std::shared_ptr<Domain::IMarkdownParser>  m_parser;
    };
}
