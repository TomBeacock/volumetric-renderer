#include "raw_importer.h"

#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>
#include <nfd.h>

#include "file_parser.h"

typedef Vol::VolumetricDataset::Type DataType;

void Vol::UI::RawImporter::update() {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 8.0f));

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, 0, ImVec2(0.5f, 0.5f));
    ImGuiWindowFlags window_flags =
        ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove;

    // Begin popup
    if (ImGui::BeginPopupModal("Import Raw", nullptr, window_flags)) {
        // Filepath
        static std::string filepath;
        ImGui::InputText("##filepath", &filepath);

        ImGui::SameLine();

        if (ImGui::Button("Browse...")) {
            nfdchar_t *out_path;
            nfdfilteritem_t filter_items[1] = {{"Raw data", "raw"}};
            if (NFD_OpenDialog(&out_path, filter_items, 1, nullptr) ==
                NFD_OKAY) {
                filepath = out_path;
                NFD_FreePath(out_path);
            }
        }

        // Dimensions
        static int v[3] = {0};
        ImGui::InputInt3("Dimensions", v);

        // Data type
        std::vector<Vol::VolumetricDataset::Type> types = {
            DataType::Int8,    DataType::Int16,   DataType::Int32,
            DataType::UInt8,   DataType::UInt16,  DataType::UInt32,
            DataType::Float32, DataType::Float64,
        };
        static size_t selected_data_type = 0;
        std::string preview_label =
            Vol::VolumetricDataset::type_to_string(types[selected_data_type]);

        if (ImGui::BeginCombo("Data type", preview_label.c_str())) {
            for (size_t i = 0; i < types.size(); i++) {
                bool is_selected = selected_data_type == i;
                std::string label =
                    Vol::VolumetricDataset::type_to_string(types[i]);
                if (ImGui::Selectable(label.c_str(), is_selected)) {
                    selected_data_type = i;
                }

                if (is_selected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }

        if (ImGui::Button("Cancel")) {
            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();

        if (ImGui::Button("Ok")) {
            ImGui::CloseCurrentPopup();
        }

        // End popup
        ImGui::EndPopup();
    }

    ImGui::PopStyleVar();
}

void Vol::UI::RawImporter::show() {
    ImGui::OpenPopup("Import Raw");
}
