#include "MusicPlayer.hpp"

#include "Constants.hpp"

void MusicPlayer::drawInterface(){
    isAnyWidgetHovered_ = false;
    
    const int screenWidth{Constants::System::WindowWidth};
    const int screenHeight{Constants::System::WindowHeight};
    
    /* white background */ {
        DrawRectangle(0, 0, screenWidth, screenHeight, ColorAlpha(WHITE, Constants::UI::BackgroundOpacity));
    } /* white background */
    
    /* Window controls */ {
        const int closeXPosition{screenWidth - Constants::UI::ButtonSize - Constants::UI::WindowControlSpacing};
        const int minimizeXPosition{closeXPosition - Constants::UI::ButtonSize - Constants::UI::WindowControlSpacing};
        
        Rectangle minimizeRectangle{
            static_cast<float>(minimizeXPosition), 
            static_cast<float>(Constants::UI::WindowControlYPosition), 
            static_cast<float>(Constants::UI::ButtonSize), 
            static_cast<float>(Constants::UI::ButtonSize)
        };
        Rectangle closeRectangle{
            static_cast<float>(closeXPosition), 
            static_cast<float>(Constants::UI::WindowControlYPosition), 
            static_cast<float>(Constants::UI::ButtonSize), 
            static_cast<float>(Constants::UI::ButtonSize)
        };
        
        if(drawImageButton(Constants::Icons::Id::Minimize, minimizeRectangle)) minimizeClicked();
        if(drawImageButton(Constants::Icons::Id::Close, closeRectangle)) closeClicked();
    } /* Window controls */
    
    /* Song Information */ {
        Vector2 titlePosition{
            static_cast<float>(Constants::UI::TextIndentation), 
            static_cast<float>(Constants::UI::TitleYPosition)
        };
        DrawText(
            displayedMusicTitle_.c_str(), 
            static_cast<int>(titlePosition.x), 
            static_cast<int>(titlePosition.y), 
            Constants::UI::TextFontSize, 
            BLACK
        );
        
        Vector2 mousePosition{GetMousePosition()};
        int titleWidth{MeasureText(displayedMusicTitle_.c_str(), Constants::UI::TextFontSize)};
        Rectangle titleRectangle{titlePosition.x, titlePosition.y, static_cast<float>(titleWidth), Constants::UI::TextFontSize};
        if(CheckCollisionPointRec(mousePosition, titleRectangle)){
            isAnyWidgetHovered_ = true;
            if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) copyMusicTitleClicked();
        }
        
        Vector2 subtitlePosition{
            static_cast<float>(Constants::UI::TextIndentation), 
            static_cast<float>(Constants::UI::SubtitleYPosition)
        };
        const char* subtitleText{isShowingArtist_ && !displayedArtistName_.empty() 
            ? displayedArtistName_.c_str() 
            : displayedFilePath_.c_str()};
        DrawText(
            subtitleText, 
            static_cast<int>(subtitlePosition.x), 
            static_cast<int>(subtitlePosition.y), 
            Constants::UI::TextFontSize, 
            ColorAlpha(BLACK, .5f)
        );
        
        int subtitleWidth{MeasureText(subtitleText, Constants::UI::TextFontSize)};
        Rectangle subtitleRectangle{subtitlePosition.x, subtitlePosition.y, static_cast<float>(subtitleWidth), Constants::UI::TextFontSize};
        if(CheckCollisionPointRec(mousePosition, subtitleRectangle)){
            isAnyWidgetHovered_ = true;
            if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) togglePathAndArtistClicked();
            if(IsMouseButtonPressed(MOUSE_BUTTON_MIDDLE)) goToFileClicked();
        }
    } /* Song Information */
    
    /* Music Progress Bar */ {
        const int currentTimeXPosition{(screenWidth - Constants::UI::ProgressBarWidth) / 2 + Constants::UI::CurrentTimeXOffset};
        DrawText(
            currentProgressString_.c_str(), 
            currentTimeXPosition, 
            Constants::UI::ProgressBarYPosition + Constants::UI::ProgressBarTimeTextOffset, 
            Constants::UI::TextFontSize, 
            BLACK
        );
        
        const int progressBarXPosition{(screenWidth - Constants::UI::ProgressBarWidth) / 2};
        Rectangle progressBarRectangle{
            static_cast<float>(progressBarXPosition), 
            static_cast<float>(Constants::UI::ProgressBarYPosition), 
            static_cast<float>(Constants::UI::ProgressBarWidth), 
            static_cast<float>(Constants::UI::ProgressBarHeight)
        };
        
        if(!IsMusicValid(music_)) GuiDisable();
        
        float oldProgress{musicProgress_};
        GuiSliderBar(progressBarRectangle, "", "", &musicProgress_, .0f, 1.0f);
        
        Vector2 mousePosition{GetMousePosition()};
        if(CheckCollisionPointRec(mousePosition, progressBarRectangle)){
            isAnyWidgetHovered_ = true;
            if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)){
                wasPausing_ = !IsMusicStreamPlaying(music_);
                isCurrentlyInteractingWithProgressBar_ = true;
            }
        }
        
        if(IsMouseButtonDown(MOUSE_BUTTON_LEFT) && isCurrentlyInteractingWithProgressBar_){
            PauseMusicStream(music_);
        }
        
        if(IsMouseButtonReleased(MOUSE_BUTTON_LEFT) && isCurrentlyInteractingWithProgressBar_){
            isCurrentlyInteractingWithProgressBar_ = false;
            if(musicProgress_ != oldProgress){
                progressBarClicked();
            }
            if(!wasPausing_) ResumeMusicStream(music_);
        }
        
        if(!IsMusicValid(music_)) GuiEnable();
        
        
        const int totalTimeXPosition{progressBarXPosition + Constants::UI::ProgressBarWidth + Constants::UI::TotalTimeXOffset};
        DrawText(
            totalLengthString_.c_str(), 
            totalTimeXPosition, 
            Constants::UI::ProgressBarYPosition + Constants::UI::ProgressBarTimeTextOffset, 
            Constants::UI::TextFontSize, 
            BLACK
        );
    } /* Music Progress Bar */
    
    /* Music Controls */ {
        const int totalControlWidth{
            Constants::UI::ButtonSize * Constants::UI::TotalMusicControlButtons + 
            Constants::UI::MusicControlInnerSpacing * 2 + 
            Constants::UI::MusicControlOuterSpacing * 2
        };
        const int controlsStartXPosition{(screenWidth - totalControlWidth) / 2};
        
        Rectangle shuffleRectangle{
            static_cast<float>(controlsStartXPosition), 
            static_cast<float>(Constants::UI::MusicControlsYPosition), 
            static_cast<float>(Constants::UI::ButtonSize), 
            static_cast<float>(Constants::UI::ButtonSize)
        };
        if(drawImageButton(isShuffling_ ? Constants::Icons::Id::Shuffle_On : Constants::Icons::Id::Shuffle_Off, shuffleRectangle)){
            toggleShuffleClicked();
        }
        
        Rectangle previousRectangle{
            static_cast<float>(controlsStartXPosition + Constants::UI::ButtonSize + Constants::UI::MusicControlOuterSpacing), 
            static_cast<float>(Constants::UI::MusicControlsYPosition), 
            static_cast<float>(Constants::UI::ButtonSize), 
            static_cast<float>(Constants::UI::ButtonSize)
        };
        if(drawImageButton(Constants::Icons::Id::Previous_Music, previousRectangle)){
            previousSongClicked();
        }
        
        Rectangle playPauseRectangle{
            static_cast<float>(controlsStartXPosition + Constants::UI::ButtonSize * 2 + Constants::UI::MusicControlOuterSpacing + Constants::UI::MusicControlInnerSpacing), 
            static_cast<float>(Constants::UI::MusicControlsYPosition), 
            static_cast<float>(Constants::UI::ButtonSize), 
            static_cast<float>(Constants::UI::ButtonSize)
        };
        Constants::Icons::Id playPauseIcon{
            (IsMusicValid(music_) && IsMusicStreamPlaying(music_)) 
                ? Constants::Icons::Id::Pause : Constants::Icons::Id::Play
        };
        if(drawImageButton(playPauseIcon, playPauseRectangle)){
            playPauseMusicClicked();
        }
        
        Rectangle nextRectangle{
            static_cast<float>(controlsStartXPosition + Constants::UI::ButtonSize * 3 + Constants::UI::MusicControlOuterSpacing + Constants::UI::MusicControlInnerSpacing * 2), 
            static_cast<float>(Constants::UI::MusicControlsYPosition), 
            static_cast<float>(Constants::UI::ButtonSize), 
            static_cast<float>(Constants::UI::ButtonSize)
        };
        if(drawImageButton(Constants::Icons::Id::Next_Music, nextRectangle)){
            nextSongClicked();
        }
        
        Constants::Icons::Id loopIcon;
        switch(loopMode_){
            case Constants::LoopMode::No_Loop:                 loopIcon = Constants::Icons::Id::No_Loop; break;
            case Constants::LoopMode::Single_Music_Loop:       loopIcon = Constants::Icons::Id::Single_Music_Loop; break;
            case Constants::LoopMode::Directory_Loop:          loopIcon = Constants::Icons::Id::Directory_Loop; break;
            case Constants::LoopMode::Directory_Loop_Infinite: loopIcon = Constants::Icons::Id::Directory_Loop_Infinite; break;
        }
        
        Rectangle loopRectangle{
            static_cast<float>(controlsStartXPosition + Constants::UI::ButtonSize * 4 + Constants::UI::MusicControlOuterSpacing * 2 + Constants::UI::MusicControlInnerSpacing * 2), 
            static_cast<float>(Constants::UI::MusicControlsYPosition), 
            static_cast<float>(Constants::UI::ButtonSize), 
            static_cast<float>(Constants::UI::ButtonSize)
        };
        if(drawImageButton(loopIcon, loopRectangle)) toggleLoopClicked();
        
        Vector2 mousePosition{GetMousePosition()};
        if(CheckCollisionPointRec(mousePosition, loopRectangle) && IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)){
            toggleLoopClicked(false);
        }
    } /* Music Controls */
}