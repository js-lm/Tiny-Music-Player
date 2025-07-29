#pragma once

#include "Constants.hpp"

#include <rlImGui.h>
#include <imgui.h>

#include <string>

bool MusicPlayer::drawImageButton(Constants::Icons::Id iconId){
    ImGui::PushID(static_cast<int>(iconId));

    const ImVec2 positionBeforeAddingButton{ImGui::GetCursorPos()};

    ImGui::SetCursorPos(ImVec2{
        positionBeforeAddingButton.x - Constants::Icons::IconOffset.x,
        positionBeforeAddingButton.y - Constants::Icons::IconOffset.y
    });
    
    bool isClicked{ImGui::InvisibleButton(
        std::to_string(static_cast<int>(iconId)).c_str(), 
        Constants::Icons::ButtonSize
    )};
    

    int offsetY{0};
    if(ImGui::IsItemActive()) offsetY = Constants::Icons::IconSize.y * 2;
    else if(ImGui::IsItemHovered()) offsetY = Constants::Icons::IconSize.y * 1;
    
    
    ImGui::SetCursorPos(positionBeforeAddingButton);
    
    rlImGuiImageRect(
        &iconsTexture_, 
        Constants::Icons::IconSize.x, 
        Constants::Icons::IconSize.y,  
        Rectangle{
            static_cast<float>(Constants::Icons::IconSize.x * static_cast<int>(iconId)),
            static_cast<float>(offsetY),
            Constants::Icons::IconSize.x,
            Constants::Icons::IconSize.y
        }
    );

    ImGui::PopID();

    return isClicked;
}