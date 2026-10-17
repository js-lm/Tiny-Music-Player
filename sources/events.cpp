#include "music_player.hpp"

#include "lock.hpp"

#include <raylib.h>
#include <raymath.h>

void MusicPlayer::handleWindowDrag(){
    Vector2 currentMouseWindowPosition{GetMousePosition()};

    if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !isAnyWidgetHovered_){
        isDragging_ = true;
        currentWindowPosition_ = GetWindowPosition();
        previousMouseScreenPosition_ = Vector2Add(
            currentWindowPosition_, currentMouseWindowPosition
        );
    }

    if(IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) isDragging_ = false;
    
    if(isDragging_ && !Vector2Equals({.0f, .0f}, GetMouseDelta())){
        Vector2 currentMouseScreenPosition{Vector2Add(
            currentWindowPosition_, currentMouseWindowPosition
        )};
        Vector2 mouseScreenDelta{Vector2Subtract(
            currentMouseScreenPosition, previousMouseScreenPosition_
        )};
        previousMouseScreenPosition_ = currentMouseScreenPosition;
        
        currentWindowPosition_ = Vector2Add(currentWindowPosition_, mouseScreenDelta);
        
        SetWindowPosition(
            static_cast<int>(currentWindowPosition_.x),
            static_cast<int>(currentWindowPosition_.y)
        );
        
    }
}

void MusicPlayer::handleFileDrop(){
    if(!IsFileDropped()) return;

    FilePathList droppedFiles{LoadDroppedFiles()};    
    initializeMusicStream(droppedFiles.paths[0], true);
    UnloadDroppedFiles(droppedFiles);
}

void MusicPlayer::handleMusicEnd(){
    switch(loopMode_){
    case constants::LoopMode::No_Loop:{
        // PauseAudioStream(audioStream_);
        isManuallyPaused_ = true;
        isAtEndOfMusic_ = true;
#if defined(__linux__)
        if(mprisIntegration_) mprisIntegration_->updatePlaybackStatus(false);
#endif
    } return;
    case constants::LoopMode::Single_Music_Loop:{
        av_seek_frame(formatContext_, -1, 0, AVSEEK_FLAG_BACKWARD);
        avcodec_flush_buffers(codecContext_);
        audioBuffer_.clear();
        PlayAudioStream(audioStream_);
    } return;
    case constants::LoopMode::Directory_Loop_Infinite:{
        findNextValidMusic(true, true);
    } return;
    case constants::LoopMode::Directory_Loop:{
        if(!findNextValidMusic(true, false)){
            isManuallyPaused_ = true;
            isAtEndOfMusic_ = true;
#if defined(__linux__)
            if(mprisIntegration_) mprisIntegration_->updatePlaybackStatus(false);
#endif
        }
    } return;

    }
}

void MusicPlayer::handleKeyboard(){
    if(IsKeyPressed(KEY_SPACE)) playPauseMusicClicked();

    int pressedSeekKey{KEY_NULL};
    if(IsKeyDown(KEY_RIGHT) && !IsKeyDown(KEY_LEFT)) pressedSeekKey = KEY_RIGHT;
    else if(IsKeyDown(KEY_LEFT) && !IsKeyDown(KEY_RIGHT)) pressedSeekKey = KEY_LEFT;

    double currentTime{GetTime()};
    bool shouldSeek{false};
    if(pressedSeekKey != heldSeekKey_){
        heldSeekKey_ = pressedSeekKey;
        seekKeyHoldStartTime_ = currentTime;
        lastSeekTime_ = currentTime;
        shouldSeek = (pressedSeekKey != KEY_NULL);
    }else if(pressedSeekKey != KEY_NULL){
        double holdDuration{currentTime - seekKeyHoldStartTime_};
        double repeatInterval{
            holdDuration < constants::system::SeekRepeatSlowPhaseSeconds
                ? constants::system::SeekRepeatSlowIntervalSeconds
                : constants::system::SeekRepeatFastIntervalSeconds
        };
        if(currentTime - lastSeekTime_ >= repeatInterval){
            lastSeekTime_ = currentTime;
            shouldSeek = true;
        }
    }

    bool isRightPressed{pressedSeekKey == KEY_RIGHT};
    
    if(shouldSeek && formatContext_ != nullptr){
        bool isShiftHeld{IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)};

        float stepSeconds{isShiftHeld ? constants::system::SeekStepFineSeconds : constants::system::SeekStepSeconds};
        float direction{isRightPressed ? 1.0f : -1.0f};
        float targetSeconds{musicTimePlayed_ + direction * stepSeconds};

        targetSeconds = Clamp(targetSeconds, .0f, currentMusicTotalLength_);

        seekToSeconds(targetSeconds);
    }
}


void MusicPlayer::handleNewInstanceOpened(){
    // if(timeSinceLastLockUpdate_ <= 0){
    //     timeSinceLastLockUpdate_ = constants::LockUpdateFrequency;

    if(auto newMusic{lock::TryGetNewFilePath()}){
        if(isMediaFile(newMusic.value().c_str())){
            initializeMusicStream(newMusic.value().c_str(), true);
        }
    }

        // lock::UpdateLockTimeStamp();
    // }
    // timeSinceLastLockUpdate_ -= GetFrameTime();
}