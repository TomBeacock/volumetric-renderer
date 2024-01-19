#include "importer.h"

#include "application.h"
#include "data/file_parser.h"
#include "rendering/offscreen_pass.h"
#include "rendering/vulkan_context.h"
#include "ui/ui_context.h"

#include <nfd.h>

#include <memory>

std::optional<std::filesystem::path> open_file_dialog(
    std::vector<nfdfilteritem_t> filters);

std::optional<std::vector<std::filesystem::path>> open_multifile_dialog(
    std::vector<nfdfilteritem_t> filters);

void Vol::Data::Importer::import(FileFormat file_format) const
{
    std::unique_ptr<FileParser> file_parser;
    switch (file_format) {
        case FileFormat::Nrrd: {
            if (auto filepath = open_file_dialog(
                    {{"Nearly Raw Raster Data", "nffd,nhdr"}})) {
                file_parser = std::make_unique<NrrdFileParser>(*filepath);
            } else {
                return;
            }
        }
    }
    try {
        Dataset dataset = file_parser->parse();
        Application::main()
            .get_vulkan_context()
            .get_offscreen_pass()
            ->volume_dataset_changed(dataset);
    } catch (std::exception &e) {
        Application::main().get_ui().show_error("Import Error", e.what());
    }
}

std::optional<std::filesystem::path> open_file_dialog(
    std::vector<nfdfilteritem_t> filters)
{
    nfdchar_t *out_path = nullptr;
    if (NFD_OpenDialog(&out_path, filters.data(), filters.size(), nullptr) ==
        NFD_OKAY) {
        std::filesystem::path path(out_path);
        NFD_FreePath(out_path);
        return path;
    }
    return std::nullopt;
}

std::optional<std::vector<std::filesystem::path>> open_multifile_dialog(
    std::vector<nfdfilteritem_t> filters)
{
    const nfdpathset_t *out_paths = nullptr;
    if (NFD_OpenDialogMultiple(
            &out_paths, filters.data(), filters.size(), nullptr) == NFD_OKAY) {
        nfdpathsetsize_t count;
        NFD_PathSet_GetCount(out_paths, &count);
        std::vector<std::filesystem::path> paths(count);
        nfdchar_t *out_path;
        for (size_t i = 0; i < count; i++) {
            NFD_PathSet_GetPath(out_paths, i, &out_path);
            paths[i] = std::filesystem::path(out_path);
            NFD_FreePath(out_path);
        }
        NFD_PathSet_Free(out_paths);
        return paths;
    }
    return std::nullopt;
}
