#include "MusicPlayer.hpp"

#include "Lock.hpp"

#include <raylib.h>
#include <raymath.h>

void MusicPlayer::handleWindowDrag(){
    Vector2 currentMouseWindowPosition{GetMousePosition()};

    if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !ImGui::IsAnyItemHovered()){
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
    initMusicStream(droppedFiles.paths[0]);
    UnloadDroppedFiles(droppedFiles);
}

void MusicPlayer::handleMusicEnd(){
    switch(loopMode_){
    case Constants::LoopMode::No_Loop:{
        PauseMusicStream(music_); 
    } return;
    case Constants::LoopMode::Single_Music_Loop: return;
    case Constants::LoopMode::Directory_Loop_Infinite:{
        goToNextMusic();
    } return;
    case Constants::LoopMode::Directory_Loop:{
        if(currentDirectoryIndex_ && startingIndex_){
            goToNextMusic();

            if(currentDirectoryIndex_.value() % static_cast<int>(musicDirectory_.count) == startingIndex_.value()){
                PauseMusicStream(music_);
                isManuallyPaused_ = true;
            }
        }
    }
    }

}

void MusicPlayer::handleKeyboard(){
    if(IsKeyPressed(KEY_SPACE)) playPauseMusicClicked();
}

void MusicPlayer::handleNewInstanceOpened(){
    if(timeSinceLastLockUpdate_ <= 0){
        timeSinceLastLockUpdate_ = Constants::LockUpdateFrequency;

        if(auto newMusic{Lock::TryGetNewFilePath()}){
            if(isMusicFile(newMusic.value().c_str())){
                initMusicStream(newMusic.value().c_str());
            }
        }

        Lock::UpdateLockTimeStamp();
    }
    timeSinceLastLockUpdate_ -= GetFrameTime();
}