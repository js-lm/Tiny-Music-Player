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
    // if(IsMusicValid(music_)){
    //     if(GetMusicTimePlayed(music_) < 1.0f){
    if(formatContext_ != nullptr){
        if(musicProgress_ * currentMusicTotalLength_ < 1.0f){
            goToPreviousMusic();
        }else{
            // PlayMusicStream(music_);
            av_seek_frame(formatContext_, -1, 0, AVSEEK_FLAG_BACKWARD);
            avcodec_flush_buffers(codecContext_);
            audioBuffer_.clear();
            currentProgressString_ = secondInFloatToString(.0f);
        }
    }
}

void MusicPlayer::playPauseMusicClicked(){
    if(IsAudioStreamPlaying(audioStream_)){
        PauseAudioStream(audioStream_);
        isManuallyPaused_ = true;
    }else{
        ResumeAudioStream(audioStream_);
        if(!IsAudioStreamPlaying(audioStream_) && formatContext_ != nullptr) PlayAudioStream(audioStream_);
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

    // music_.looping = loopMode_ == Constants::LoopMode::Single_Music_Loop;
}

void MusicPlayer::progressBarClicked(){
    if(formatContext_ != nullptr){
        int64_t targetPts{static_cast<int64_t>((musicProgress_ * currentMusicTotalLength_) * AV_TIME_BASE)};
        av_seek_frame(formatContext_, -1, targetPts, AVSEEK_FLAG_BACKWARD);
        avcodec_flush_buffers(codecContext_);
        audioBuffer_.clear();
        currentProgressString_ = secondInFloatToString(musicProgress_ * currentMusicTotalLength_);
    }
}

void MusicPlayer::togglePathAndArtistClicked(){
    if(isShowingArtist_) isShowingArtist_ = false;
    else if(!displayedArtistName_.empty()) isShowingArtist_ = true;
}