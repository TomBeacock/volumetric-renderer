#include "importer.h"

#include "application.h"
#include "data/file_parser.h"
#include "rendering/offscreen_pass.h"
#include "rendering/vulkan_context.h"
#include "ui/ui_context.h"

#include <memory>

void Vol::Data::Importer::import(const std::filesystem::path &filepath) const
{
    try {
        if (!filepath.has_extension()) {
            throw std::runtime_error("Failed to determine file format");
        }

        std::unique_ptr<FileParser> file_parser;
        if (filepath.extension() == ".nrrd" ||
            filepath.extension() == ".ndhr") {
            file_parser = std::make_unique<RawFileParser>(filepath);
        }

        if (!file_parser) {
            throw std::runtime_error("Unsupported file format");
        }

        Dataset dataset = file_parser->parse();
        Application::main()
            .get_vulkan_context()
            .get_offscreen_pass()
            ->volume_dataset_changed(dataset);
    } catch (std::exception &e) {
        Application::main().get_ui().show_error("Import Error", e.what());
    }
}