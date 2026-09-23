#include "music_player.hpp"

#include "debug_utilities.hpp"

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
    if(isShuffling_) playedFiles_.clear();
}

void MusicPlayer::previousSongClicked(){
    // if(IsMusicValid(music_)){
    //     if(GetMusicTimePlayed(music_) < 1.0f){
    if(formatContext_ != nullptr){
        if(musicProgress_ * currentMusicTotalLength_ < 1.0f){
            findNextValidMusic(false);
        }else{
            // PlayMusicStream(music_);
            av_seek_frame(formatContext_, -1, 0, AVSEEK_FLAG_BACKWARD);
            avcodec_flush_buffers(codecContext_);
            audioBuffer_.clear();
            musicTimePlayed_ = .0f;
            PlayAudioStream(audioStream_);
            currentProgressString_ = secondInFloatToString(.0f);
        }
    }
}

void MusicPlayer::playPauseMusicClicked(){
    if(IsAudioStreamPlaying(audioStream_)){
        PauseAudioStream(audioStream_);
        isManuallyPaused_ = true;
    }else{
        if(musicProgress_ >= 0.999f && formatContext_ != nullptr){
            if(loopMode_ == constants::LoopMode::Directory_Loop){
                findNextValidMusic(true, true);
            }else{
                av_seek_frame(formatContext_, -1, 0, AVSEEK_FLAG_BACKWARD);
                avcodec_flush_buffers(codecContext_);
                audioBuffer_.clear();
                musicTimePlayed_ = .0f;
                currentProgressString_ = secondInFloatToString(.0f);
            }
        }
        ResumeAudioStream(audioStream_);
        if(!IsAudioStreamPlaying(audioStream_) && formatContext_ != nullptr) PlayAudioStream(audioStream_);
        isManuallyPaused_ = false;
    }
}

void MusicPlayer::nextSongClicked(){
    findNextValidMusic(true);
}

void MusicPlayer::toggleLoopClicked(bool isForward){
    int currentLoopModeIndex{static_cast<int>(loopMode_)};

    if(isForward){
        loopMode_ = static_cast<constants::LoopMode>((currentLoopModeIndex + 1) % constants::NumberOfLoopMode);
    }else{
        loopMode_ = static_cast<constants::LoopMode>((currentLoopModeIndex - 1 + constants::NumberOfLoopMode) % constants::NumberOfLoopMode);
    }

    // music_.looping = loopMode_ == constants::LoopMode::Single_Music_Loop;
}

void MusicPlayer::progressBarClicked(){
    if(formatContext_ != nullptr){
        int64_t targetPresentationTimestamp{static_cast<int64_t>(
            (musicProgress_ * currentMusicTotalLength_)
          / av_q2d(formatContext_->streams[audioStreamIndex_]->time_base)
        )};
        
        DEBUG_PRINT("[progressBarClicked] musicProgress_={:.4f} targetTime={:.4f}", musicProgress_, musicProgress_ * currentMusicTotalLength_);
        
        int seekResult{av_seek_frame(formatContext_, audioStreamIndex_, targetPresentationTimestamp, AVSEEK_FLAG_ANY)};
        DEBUG_PRINT("[progressBarClicked] av_seek_frame returned {}", seekResult);
        
        avcodec_flush_buffers(codecContext_);
        swr_init(swrContext_);
        audioBuffer_.clear();
        musicTimePlayed_ = musicProgress_ * currentMusicTotalLength_;
        currentProgressString_ = secondInFloatToString(musicTimePlayed_);
        seekGeneration_++;
        
        bool wasPlaying{IsAudioStreamPlaying(audioStream_)};
        StopAudioStream(audioStream_);
        if(wasPlaying) PlayAudioStream(audioStream_);
    }
}

void MusicPlayer::togglePathAndArtistClicked(){
    if(isShowingArtist_) isShowingArtist_ = false;
    else if(!displayedArtistName_.empty()) isShowingArtist_ = true;
}