#include "MusicPlayer.hpp"

#include "DrawImageButton.hpp"

#include <rlImGui.h>

void MusicPlayer::drawInterface(){
    ImGui::SetNextWindowSize(ImVec2{static_cast<float>(GetScreenWidth()), static_cast<float>(GetScreenHeight())});
    ImGui::SetNextWindowPos(ImVec2{0, 0});

    if(ImGui::Begin(
        "tiny-music-player", nullptr,
        ImGuiWindowFlags_NoTitleBar 
      | ImGuiWindowFlags_NoResize 
      | ImGuiWindowFlags_NoScrollbar 
      | ImGuiWindowFlags_NoScrollWithMouse 
      | ImGuiWindowFlags_NoCollapse
      | ImGuiWindowFlags_AlwaysAutoResize 
      | ImGuiWindowFlags_NoSavedSettings
    )){
        
        /* Window controls */ 
        if(ImGui::BeginTable("window-controls-table", 2, ImGuiTableFlags_NoPadOuterX | ImGuiTableFlags_NoPadInnerX, ImVec2(-1, 0))){
            ImGui::TableSetupColumn("left-stretch", ImGuiTableColumnFlags_WidthStretch, 0);
            ImGui::TableSetupColumn("content", ImGuiTableColumnFlags_WidthFixed, 0);
            ImGui::TableNextRow(0, 0);
            ImGui::TableSetColumnIndex(1);

            if(drawImageButton(Constants::Icons::Id::Minimize)) minimizeClicked();
            if(ImGui::IsItemHovered()) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);

            ImGui::SameLine(0, Constants::ImGui::ControlPanelHorizontalSpacing);
            if(drawImageButton(Constants::Icons::Id::Close)) closeClicked();
            if(ImGui::IsItemHovered()) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);

            ImGui::EndTable();
        } /* Window controls */

        /* Song Information */ {
            ImGui::Dummy(ImVec2(0, ImGui::GetStyle().ItemSpacing.y));
            ImGui::SameLine(0, Constants::ImGui::ProgressBarIndentation);
            ImGui::TextUnformatted(displayedMusicTitle_.c_str());
            if(ImGui::IsItemHovered()) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
            if(ImGui::IsItemClicked()) copyMusicTitleClicked();

            ImGui::Dummy(ImVec2(0, ImGui::GetStyle().ItemSpacing.y));
            ImGui::SameLine(0, Constants::ImGui::ProgressBarIndentation);
            ImGui::PushStyleColor(ImGuiCol_Text, Constants::ImGui::SubtitleColor);
            if(isShowingArtist_ && !displayedArtistName_.empty()){
                ImGui::TextUnformatted(displayedArtistName_.c_str());
            }else{
                ImGui::TextUnformatted(displayedFilePath_.c_str());
                if(ImGui::IsItemClicked(ImGuiMouseButton_Middle)) goToFileClicked();
            }
            if(ImGui::IsItemClicked()) togglePathAndArtistClicked();
            if(ImGui::IsItemHovered() && !displayedArtistName_.empty()){
                ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
            }
            ImGui::PopStyleColor();
        } /* Song Information */

        /* Music Progress Bar */ {
            ImGui::Dummy(ImVec2(0, ImGui::GetStyle().ItemSpacing.y));
            if(ImGui::BeginTable("progress-bar-table", 3, ImGuiTableFlags_NoPadOuterX | ImGuiTableFlags_NoPadInnerX, ImVec2(-1, 0))){
                ImGui::TableSetupColumn("left-stretch", ImGuiTableColumnFlags_WidthStretch, 0);
                ImGui::TableSetupColumn("content", ImGuiTableColumnFlags_WidthFixed, 0);
                ImGui::TableSetupColumn("right-stretch", ImGuiTableColumnFlags_WidthStretch, 0);
                ImGui::TableNextRow(0, 0);
                ImGui::TableSetColumnIndex(1);

                ImGui::TextUnformatted(currentProgressString_.c_str());

                if(!IsMusicValid(music_)) ImGui::BeginDisabled();
                ImGui::SameLine(0, Constants::ImGui::ProgressBarHorizontalSpacing);
                ImGui::SetNextItemWidth(scaleToDpiFloat(Constants::ImGui::ProgressBarWidth));
                if((isCurrentlyInteractingWithProgressBar_ = ImGui::SliderFloat("##music-progress", &musicProgress_, .0f, 1.0f, ""))) progressBarClicked();
                if(ImGui::IsItemClicked()) wasPausing_ = !IsMusicStreamPlaying(music_);
                if(ImGui::IsItemActive()) PauseMusicStream(music_);
                if(ImGui::IsItemDeactivatedAfterEdit() && !wasPausing_) ResumeMusicStream(music_);
                if(ImGui::IsItemHovered()) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
                if(!IsMusicValid(music_)) ImGui::EndDisabled();

                ImGui::SameLine(0, Constants::ImGui::ProgressBarHorizontalSpacing);
                ImGui::TextUnformatted(totalLengthString_.c_str());

                ImGui::EndTable();
            }
        } /* Music Progress Bar */

        /* Music Controls */ {
            ImGui::Dummy(ImVec2(0, ImGui::GetStyle().ItemSpacing.y));
            if(ImGui::BeginTable("music-control-table", 3, ImGuiTableFlags_NoPadOuterX | ImGuiTableFlags_NoPadInnerX, ImVec2(-1, 0))) {
                ImGui::TableSetupColumn("left-stretch", ImGuiTableColumnFlags_WidthStretch, 0);
                ImGui::TableSetupColumn("content", ImGuiTableColumnFlags_WidthFixed, 0);
                ImGui::TableSetupColumn("right-stretch", ImGuiTableColumnFlags_WidthStretch, 0);
                ImGui::TableNextRow(0, 0);
                ImGui::TableSetColumnIndex(1);

                if(drawImageButton((isShuffling_ ? Constants::Icons::Id::Shuffle_On : Constants::Icons::Id::Shuffle_Off))) toggleShuffleClicked();
                if(ImGui::IsItemHovered()) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);

                ImGui::SameLine(0, scaleToDpiFloat(Constants::ImGui::MusicControlOuterHorizontalSpacing));
                if(drawImageButton(Constants::Icons::Id::Previous_Music)) previousSongClicked();
                if(ImGui::IsItemHovered()) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);


                ImGui::SameLine(0, scaleToDpiFloat(Constants::ImGui::MusicControlInnerHorizontalSpacing));
                if(drawImageButton(IsMusicValid(music_) && IsMusicStreamPlaying(music_) ? Constants::Icons::Id::Pause : Constants::Icons::Id::Play)) playPauseMusicClicked();
                if(ImGui::IsItemHovered()) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);


                ImGui::SameLine(0, scaleToDpiFloat(Constants::ImGui::MusicControlInnerHorizontalSpacing));
                if(drawImageButton(Constants::Icons::Id::Next_Music)) nextSongClicked();
                if(ImGui::IsItemHovered()) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);

                Constants::Icons::Id iconId;
                switch(loopMode_){
                case Constants::LoopMode::No_Loop:                 iconId = Constants::Icons::Id::No_Loop; break;
                case Constants::LoopMode::Single_Music_Loop:       iconId = Constants::Icons::Id::Single_Music_Loop; break;
                case Constants::LoopMode::Directory_Loop:          iconId = Constants::Icons::Id::Directory_Loop; break;
                case Constants::LoopMode::Directory_Loop_Infinite: iconId = Constants::Icons::Id::Directory_Loop_Infinite; break;
                }

                ImGui::SameLine(0, scaleToDpiFloat(Constants::ImGui::MusicControlOuterHorizontalSpacing));

                if(drawImageButton(iconId)) toggleLoopClicked();
                if(ImGui::IsItemHovered()){
                    ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
                    
                    if(ImGui::IsMouseClicked(ImGuiMouseButton_Right)){
                        toggleLoopClicked(false);
                    }

                    switch(loopMode_){
                    case Constants::LoopMode::No_Loop: 
                        ImGui::SetTooltip("Looping is off"); 
                        break;
                    case Constants::LoopMode::Single_Music_Loop: 
                        ImGui::SetTooltip("Loop the current song"); 
                        break;
                    case Constants::LoopMode::Directory_Loop: 
                        ImGui::SetTooltip("Loop all songs in the current directory"); 
                        break;
                    case Constants::LoopMode::Directory_Loop_Infinite: 
                        ImGui::SetTooltip("Loop all songs in the current directory infinitely"); 
                        break;
                    }
                }

                ImGui::EndTable();
            }
        } /* Music Controls */
        
        ImGui::End();
    }
}