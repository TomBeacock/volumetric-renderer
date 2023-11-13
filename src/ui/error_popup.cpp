#include "error_popup.h"

#include <imgui.h>

void Vol::UI::ErrorPopup::update()
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 8.0f));

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(300.0f, 0.0f));

    std::string popup_name = title + "###error_popup";
    if (ImGui::BeginPopupModal(popup_name.c_str(), nullptr)) {
        ImGui::Text(message.c_str());

        ImGui::Dummy(ImVec2(0.0f, 8.0f));

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
        ImGui::Dummy(ImVec2(ImGui::GetContentRegionAvail().x - 50.0f, 0.0f));
        ImGui::SameLine();

        if (ImGui::Button("Ok", ImVec2(50.0f, 0.0f))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::PopStyleVar();

        ImGui::EndPopup();
    }

    ImGui::PopStyleVar();
}

void Vol::UI::ErrorPopup::show(
    const std::string &title,
    const std::string &message)
{
    this->title = title;
    this->message = message;

    ImGui::OpenPopup("###error_popup");
}
