#include "music_player.hpp"

#include "constants.hpp"

void MusicPlayer::drawInterface(){
    std::lock_guard<std::recursive_mutex> lock{musicMutex_};
    isAnyWidgetHovered_ = false;
    std::string activeTooltip;
    
    const int screenWidth{constants::system::WindowWidth};
    const int screenHeight{constants::system::WindowHeight};
    
    /* white background */ {
        DrawRectangle(0, 0, screenWidth, screenHeight, ColorAlpha(WHITE, constants::ui::BackgroundOpacity));
    } /* white background */
    
    /* Window controls */ {
        const int closeXPosition{screenWidth - constants::ui::ButtonSize - constants::ui::WindowControlSpacing};
        const int minimizeXPosition{closeXPosition - constants::ui::ButtonSize - constants::ui::WindowControlSpacing};
        
        Rectangle minimizeRectangle{
            static_cast<float>(minimizeXPosition), 
            static_cast<float>(constants::ui::WindowControlYPosition), 
            static_cast<float>(constants::ui::ButtonSize), 
            static_cast<float>(constants::ui::ButtonSize)
        };
        Rectangle closeRectangle{
            static_cast<float>(closeXPosition), 
            static_cast<float>(constants::ui::WindowControlYPosition), 
            static_cast<float>(constants::ui::ButtonSize), 
            static_cast<float>(constants::ui::ButtonSize)
        };
        
        if(drawImageButton(constants::icons::Id::Minimize, minimizeRectangle)) minimizeClicked();
        if(drawImageButton(constants::icons::Id::Close, closeRectangle)) closeClicked();
    } /* Window controls */
    
    /* Song Information */ {
        Vector2 titlePosition{
            static_cast<float>(constants::ui::TextIndentation), 
            static_cast<float>(constants::ui::TitleYPosition)
        };
        DrawTextEx(
            customFont_,
            displayedMusicTitle_.c_str(), 
            titlePosition, 
            constants::ui::TextFontSize, 
            1.0f,
            BLACK
        );
        
        Vector2 mousePosition{GetMousePosition()};
        int titleWidth{static_cast<int>(MeasureTextEx(customFont_, displayedMusicTitle_.c_str(), constants::ui::TextFontSize, 1.0f).x)};
        Rectangle titleRectangle{titlePosition.x, titlePosition.y, static_cast<float>(titleWidth), constants::ui::TextFontSize};
        if(CheckCollisionPointRec(mousePosition, titleRectangle)){
            isAnyWidgetHovered_ = true;
            if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) copyMusicTitleClicked();
        }
        
        Vector2 subtitlePosition{
            static_cast<float>(constants::ui::TextIndentation), 
            static_cast<float>(constants::ui::SubtitleYPosition)
        };
        const char *subtitleText{isShowingArtist_ && !displayedArtistName_.empty() 
            ? displayedArtistName_.c_str() 
            : displayedFilePath_.c_str()};
        DrawTextEx(
            customFont_,
            subtitleText, 
            subtitlePosition, 
            constants::ui::TextFontSize, 
            1.0f,
            ColorAlpha(BLACK, .5f)
        );
        
        int subtitleWidth{static_cast<int>(MeasureTextEx(customFont_, subtitleText, constants::ui::TextFontSize, 1.0f).x)};
        Rectangle subtitleRectangle{subtitlePosition.x, subtitlePosition.y, static_cast<float>(subtitleWidth), constants::ui::TextFontSize};
        if(CheckCollisionPointRec(mousePosition, subtitleRectangle)){
            isAnyWidgetHovered_ = true;
            if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) togglePathAndArtistClicked();
            if(IsMouseButtonPressed(MOUSE_BUTTON_MIDDLE)) goToFileClicked();
        }
    } /* Song Information */
    
    /* Music Progress Bar */ {
        const int currentTimeXPosition{(screenWidth - constants::ui::ProgressBarWidth) / 2 + constants::ui::CurrentTimeXOffset};
        DrawTextEx(
            customFont_,
            currentProgressString_.c_str(), 
            Vector2{static_cast<float>(currentTimeXPosition), static_cast<float>(constants::ui::ProgressBarYPosition + constants::ui::ProgressBarTimeTextOffset)}, 
            constants::ui::TextFontSize, 
            1.0f,
            BLACK
        );
        
        const int progressBarXPosition{(screenWidth - constants::ui::ProgressBarWidth) / 2};
        Rectangle progressBarRectangle{
            static_cast<float>(progressBarXPosition), 
            static_cast<float>(constants::ui::ProgressBarYPosition), 
            static_cast<float>(constants::ui::ProgressBarWidth), 
            static_cast<float>(constants::ui::ProgressBarHeight)
        };
        
        // // if(!IsMusicValid(music_)) GuiDisable();
        // if(formatContext_ == nullptr) GuiDisable();
        
        Vector2 mousePosition{GetMousePosition()};
        bool isHoveringProgressBar{CheckCollisionPointRec(mousePosition, progressBarRectangle) && (formatContext_ != nullptr)};
        
        Color sliderBackgroundColor{(formatContext_ == nullptr) ? constants::ui::ProgressBarBackgroundColorDisabled : constants::ui::ProgressBarBackgroundColor};
        Color sliderProgressColor{(formatContext_ == nullptr) ? constants::ui::ProgressBarFillColorDisabled : constants::ui::ProgressBarFillColor};
        
        DrawRectangleRec(progressBarRectangle, sliderBackgroundColor);
        if(musicProgress_ > .0f){
            Rectangle progressFill{progressBarRectangle};
            progressFill.width *= musicProgress_;
            DrawRectangleRec(progressFill, sliderProgressColor);
        }
        
        if(isHoveringProgressBar && currentMusicTotalLength_ > 0){
            // GuiDisableTooltip();
            // GuiSetTooltip(nullptr);
            
            float hoveredProgress{(mousePosition.x - progressBarRectangle.x) / progressBarRectangle.width};
            if(hoveredProgress < .0f) hoveredProgress = .0f;
            if(hoveredProgress > 1.0f) hoveredProgress = 1.0f;
            
            Rectangle hoverIndicatorRectangle{
                progressBarRectangle.x + hoveredProgress * progressBarRectangle.width - constants::ui::ProgressBarHoverIndicatorXOffset,
                progressBarRectangle.y,
                constants::ui::ProgressBarHoverIndicatorWidth,
                progressBarRectangle.height
            };
            DrawRectangleRec(hoverIndicatorRectangle, WHITE);
            
            activeTooltip = secondInFloatToString(hoveredProgress * currentMusicTotalLength_);
        }
        
        if(isHoveringProgressBar){
            isAnyWidgetHovered_ = true;
            if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)){
                wasPausing_ = !IsAudioStreamPlaying(audioStream_);
                isCurrentlyInteractingWithProgressBar_ = true;
                
                // seem like GuiSliderBar does not update the value on the first press frame,
                // only on subsequent frames. So I manually set it from the mouse position here
                float clickedProgress{(mousePosition.x - progressBarRectangle.x) / progressBarRectangle.width};
                if(clickedProgress < .0f) clickedProgress = .0f;
                if(clickedProgress > 1.0f) clickedProgress = 1.0f;
                musicProgress_ = clickedProgress;
            }
        }
        
        if(IsMouseButtonDown(MOUSE_BUTTON_LEFT) && isCurrentlyInteractingWithProgressBar_){
            PauseAudioStream(audioStream_);
            float clickedProgress{(mousePosition.x - progressBarRectangle.x) / progressBarRectangle.width};
            if(clickedProgress < .0f) clickedProgress = .0f;
            if(clickedProgress > 1.0f) clickedProgress = 1.0f;
            musicProgress_ = clickedProgress;
        }
        
        if(IsMouseButtonReleased(MOUSE_BUTTON_LEFT) && isCurrentlyInteractingWithProgressBar_){
            // DEBUG_PRINT("[Interface] Released! musicProgress_={:.4f} oldProgress={:.4f}", musicProgress_, oldProgress);
            isCurrentlyInteractingWithProgressBar_ = false;
            // if(musicProgress_ != oldProgress){
                progressBarClicked();
            // }
            if(!wasPausing_) ResumeAudioStream(audioStream_);
        }
        
        // if(formatContext_ == nullptr) GuiEnable();
        
        
        const int totalTimeXPosition{progressBarXPosition + constants::ui::ProgressBarWidth + constants::ui::TotalTimeXOffset};
        DrawTextEx(
            customFont_,
            totalLengthString_.c_str(), 
            Vector2{static_cast<float>(totalTimeXPosition), static_cast<float>(constants::ui::ProgressBarYPosition + constants::ui::ProgressBarTimeTextOffset)}, 
            constants::ui::TextFontSize, 
            1.0f,
            BLACK
        );
    } /* Music Progress Bar */
    
    /* Music Controls */ {
        const int totalControlWidth{
            constants::ui::ButtonSize * constants::ui::TotalMusicControlButtons + 
            constants::ui::MusicControlInnerSpacing * 2 + 
            constants::ui::MusicControlOuterSpacing * 2
        };
        const int controlsStartXPosition{(screenWidth - totalControlWidth) / 2};
        
        Rectangle shuffleRectangle{
            static_cast<float>(controlsStartXPosition), 
            static_cast<float>(constants::ui::MusicControlsYPosition), 
            static_cast<float>(constants::ui::ButtonSize), 
            static_cast<float>(constants::ui::ButtonSize)
        };
        if(drawImageButton(isShuffling_ ? constants::icons::Id::Shuffle_On : constants::icons::Id::Shuffle_Off, shuffleRectangle)){
            toggleShuffleClicked();
        }
        
        Rectangle previousRectangle{
            static_cast<float>(controlsStartXPosition + constants::ui::ButtonSize + constants::ui::MusicControlOuterSpacing), 
            static_cast<float>(constants::ui::MusicControlsYPosition), 
            static_cast<float>(constants::ui::ButtonSize), 
            static_cast<float>(constants::ui::ButtonSize)
        };
        if(drawImageButton(constants::icons::Id::Previous_Music, previousRectangle)){
            previousSongClicked();
        }
        
        Rectangle playPauseRectangle{
            static_cast<float>(controlsStartXPosition + constants::ui::ButtonSize * 2 + constants::ui::MusicControlOuterSpacing + constants::ui::MusicControlInnerSpacing), 
            static_cast<float>(constants::ui::MusicControlsYPosition), 
            static_cast<float>(constants::ui::ButtonSize), 
            static_cast<float>(constants::ui::ButtonSize)
        };
        
        constants::icons::Id playPauseIcon{
            (formatContext_ != nullptr && IsAudioStreamPlaying(audioStream_)) 
                ? constants::icons::Id::Pause : constants::icons::Id::Play
        };
        if(drawImageButton(playPauseIcon, playPauseRectangle)){
            playPauseMusicClicked();
        }
        
        Rectangle nextRectangle{
            static_cast<float>(controlsStartXPosition + constants::ui::ButtonSize * 3 + constants::ui::MusicControlOuterSpacing + constants::ui::MusicControlInnerSpacing * 2), 
            static_cast<float>(constants::ui::MusicControlsYPosition), 
            static_cast<float>(constants::ui::ButtonSize), 
            static_cast<float>(constants::ui::ButtonSize)
        };
        if(drawImageButton(constants::icons::Id::Next_Music, nextRectangle)){
            nextSongClicked();
        }
        
        constants::icons::Id loopIcon;
        switch(loopMode_){
            case constants::LoopMode::No_Loop:                 loopIcon = constants::icons::Id::No_Loop; break;
            case constants::LoopMode::Single_Music_Loop:       loopIcon = constants::icons::Id::Single_Music_Loop; break;
            case constants::LoopMode::Directory_Loop:          loopIcon = constants::icons::Id::Directory_Loop; break;
            case constants::LoopMode::Directory_Loop_Infinite: loopIcon = constants::icons::Id::Directory_Loop_Infinite; break;
        }
        
        Rectangle loopRectangle{
            static_cast<float>(controlsStartXPosition + constants::ui::ButtonSize * 4 + constants::ui::MusicControlOuterSpacing * 2 + constants::ui::MusicControlInnerSpacing * 2), 
            static_cast<float>(constants::ui::MusicControlsYPosition), 
            static_cast<float>(constants::ui::ButtonSize), 
            static_cast<float>(constants::ui::ButtonSize)
        };
        if(drawImageButton(loopIcon, loopRectangle)) toggleLoopClicked();
        
        Vector2 mousePosition{GetMousePosition()};
        if(CheckCollisionPointRec(mousePosition, loopRectangle)){
            isAnyWidgetHovered_ = true;
            if(IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)){
                toggleLoopClicked(false);
            }
            switch(loopMode_){
            case constants::LoopMode::No_Loop:                  activeTooltip = constants::ui::TooltipNoLoop; break;
            case constants::LoopMode::Single_Music_Loop:        activeTooltip = constants::ui::TooltipSingleMusicLoop; break;
            case constants::LoopMode::Directory_Loop:           activeTooltip = constants::ui::TooltipDirectoryLoop; break;
            case constants::LoopMode::Directory_Loop_Infinite:  activeTooltip = constants::ui::TooltipDirectoryLoopInfinite; break;
            }
        }
    } /* Music Controls */
    
    if(!activeTooltip.empty()){
        Vector2 mousePosition{GetMousePosition()};
        Vector2 tooltipTextSize{MeasureTextEx(customFont_, activeTooltip.c_str(), constants::ui::TextFontSize, 1.0f)};
        float tooltipHeight{static_cast<float>(constants::ui::TextFontSize + constants::ui::TooltipHeightPadding)};
        float tooltipWidth{tooltipTextSize.x + constants::ui::TooltipWidthPadding};
        
        Rectangle tooltipRectangle{
            mousePosition.x - tooltipWidth / 2.0f,
            mousePosition.y - tooltipHeight - constants::ui::TooltipYOffset,
            tooltipWidth,
            tooltipHeight
        };
        
        if(tooltipRectangle.x < 0) tooltipRectangle.x = 0;
        if(tooltipRectangle.x + tooltipWidth > GetScreenWidth()) tooltipRectangle.x = GetScreenWidth() - tooltipWidth;
        if(tooltipRectangle.y < 0) tooltipRectangle.y = mousePosition.y + constants::ui::TooltipFallbackYOffset;
        
        DrawRectangleRec(tooltipRectangle, constants::ui::TooltipBackgroundColor);
        DrawRectangleLinesEx(tooltipRectangle, constants::ui::TooltipBorderWidth, constants::ui::TooltipBorderColor);
        
        Vector2 textPos{
            tooltipRectangle.x + (tooltipRectangle.width - tooltipTextSize.x) / 2.0f,
            tooltipRectangle.y + (tooltipRectangle.height - tooltipTextSize.y) / 2.0f
        };
        DrawTextEx(customFont_, activeTooltip.c_str(), textPos, constants::ui::TextFontSize, 1.0f, BLACK);
    }
}