#include "MusicPlayer.hpp"

#include <stdlib.h>

void MusicPlayer::minimizeClicked(){
    MinimizeWindow();
}

void MusicPlayer::closeClicked(){
    shouldClose_ = true;
}

void MusicPlayer::copyMusicTitleClicked(){
    SetClipboardText(displayedMusicTitle_.c_str());
}

void MusicPlayer::goToFileClicked(){
    if(displayedFilePath_.empty()) return;

    char command[2048];

#if defined(__linux__)
    snprintf(command, sizeof(command), "xdg-open \"%s\"", GetDirectoryPath(displayedFilePath_.c_str()));
#elif defined(__APPLE__)
    snprintf(command, sizeof(command), "open -R \"%s\"", displayedFilePath_.c_str());
#elif defined(_WIN32)
    snprintf(command, sizeof(command), "explorer /select,\"%s\"", displayedFilePath_.c_str());
#endif

    system(command);
}

void MusicPlayer::toggleShuffleClicked(){
    isShuffling_ = !isShuffling_;
    if(isShuffling_) shuffleMusic();
}

void MusicPlayer::previousSongClicked(){
    if(IsMusicValid(music_)){
        if(GetMusicTimePlayed(music_) < 1.0f){
            goToPreviousMusic();
        }else{
            StopMusicStream(music_);
            PlayMusicStream(music_);
        }
    }
}

void MusicPlayer::playPauseMusicClicked(){
    if(IsMusicStreamPlaying(music_)){
        PauseMusicStream(music_);
        isManuallyPaused_ = true;
    }else{
        ResumeMusicStream(music_);
        if(!IsMusicStreamPlaying(music_) && IsMusicValid(music_)) PlayMusicStream(music_);
        isManuallyPaused_ = false;
    }
}

void MusicPlayer::nextSongClicked(){
    goToNextMusic();
}

void MusicPlayer::toggleLoopClicked(bool isForward){
    int currentLoopModeIndex{static_cast<int>(loopMode_)};

    if(isForward){
        loopMode_ = static_cast<Constants::LoopMode>((currentLoopModeIndex + 1) % Constants::NumberOfLoopMode);
    }else{
        loopMode_ = static_cast<Constants::LoopMode>((currentLoopModeIndex - 1 + Constants::NumberOfLoopMode) % Constants::NumberOfLoopMode);
    }

    music_.looping = loopMode_ == Constants::LoopMode::Single_Music_Loop;
}

void MusicPlayer::progressBarClicked(){
    if(IsMusicValid(music_)){
        SeekMusicStream(music_, musicProgress_ * currentMusicTotalLength_);
        currentProgressString_ = secondInFloatToString(musicProgress_ * currentMusicTotalLength_);
    }
}

void MusicPlayer::togglePathAndArtistClicked(){
    if(isShowingArtist_) isShowingArtist_ = false;
    else if(!displayedArtistName_.empty()) isShowingArtist_ = true;
}