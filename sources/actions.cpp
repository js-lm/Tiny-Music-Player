#include "music_player.hpp"

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
    if(isShuffling_){
        playedFiles_.clear();
        shuffleHistory_.clear();
        if(!currentFileName_.empty()){
            shuffleHistory_.push_back(currentFileName_);
            shuffleHistoryIndex_ = 0;
            playedFiles_.insert(currentFileName_);
        }
    }
}

void MusicPlayer::previousSongClicked(){
    if(formatContext_ == nullptr) return;

    // if(IsMusicValid(music_)){
    //     if(GetMusicTimePlayed(music_) < 1.0f){
    if(formatContext_ != nullptr){
        if(musicProgress_ * currentMusicTotalLength_ < 2.0f){
            if(!findNextValidMusic(false)){
                av_seek_frame(formatContext_, -1, 0, AVSEEK_FLAG_BACKWARD);
                avcodec_flush_buffers(codecContext_);
                audioBuffer_.clear();
                musicTimePlayed_ = .0f;
                PlayAudioStream(audioStream_);
                currentProgressString_ = secondInFloatToString(.0f);
                isAtEndOfMusic_ = false;
            }
        }else{
            // PlayMusicStream(music_);
            av_seek_frame(formatContext_, -1, 0, AVSEEK_FLAG_BACKWARD);
            avcodec_flush_buffers(codecContext_);
            audioBuffer_.clear();
            musicTimePlayed_ = .0f;
            PlayAudioStream(audioStream_);
            currentProgressString_ = secondInFloatToString(.0f);
            isAtEndOfMusic_ = false;
        }
    }
}

void MusicPlayer::playPauseMusicClicked(){
    if(formatContext_ == nullptr) return;

    if(IsAudioStreamPlaying(audioStream_)){
        PauseAudioStream(audioStream_);
        isManuallyPaused_ = true;
    }else{
        if(isAtEndOfMusic_ && formatContext_ != nullptr){
            if(loopMode_ == constants::LoopMode::Directory_Loop){
                findNextValidMusic(true, true);
            }else{
                av_seek_frame(formatContext_, -1, 0, AVSEEK_FLAG_BACKWARD);
                avcodec_flush_buffers(codecContext_);
                audioBuffer_.clear();
                musicTimePlayed_ = .0f;
                currentProgressString_ = secondInFloatToString(.0f);
            }
            isAtEndOfMusic_ = false;
        }
        ResumeAudioStream(audioStream_);
        if(!IsAudioStreamPlaying(audioStream_) && formatContext_ != nullptr) PlayAudioStream(audioStream_);
        isManuallyPaused_ = false;
    }

#if defined(__linux__)
    if(mprisIntegration_){
        mprisIntegration_->updatePlaybackStatus(!isManuallyPaused_);
    }
#endif
}

void MusicPlayer::nextSongClicked(){
    if(formatContext_ == nullptr) return;
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
    seekToSeconds(musicProgress_ * currentMusicTotalLength_);
}

void MusicPlayer::seekToSeconds(float targetSeconds){
    std::lock_guard<std::recursive_mutex> lock{musicMutex_};
    if(formatContext_ != nullptr){
        int64_t targetPresentationTimestamp{static_cast<int64_t>(
            targetSeconds
          / av_q2d(formatContext_->streams[audioStreamIndex_]->time_base)
        )};
        
        // DEBUG_PRINT("[progressBarClicked] musicProgress_={:.4f} targetTime={:.4f}", musicProgress_, musicProgress_ * currentMusicTotalLength_);
        
        int seekResult{av_seek_frame(formatContext_, audioStreamIndex_, targetPresentationTimestamp, AVSEEK_FLAG_ANY)};
        // DEBUG_PRINT("[progressBarClicked] av_seek_frame returned {}", seekResult);
        
        avcodec_flush_buffers(codecContext_);
        swr_init(swrContext_);
        audioBuffer_.clear();
        musicTimePlayed_ = targetSeconds;
        currentProgressString_ = secondInFloatToString(musicTimePlayed_);
        seekGeneration_++;
        isAtEndOfMusic_ = false;
        
        bool wasPlaying{IsAudioStreamPlaying(audioStream_)};
        StopAudioStream(audioStream_);
        if(wasPlaying) PlayAudioStream(audioStream_);
    }
}

void MusicPlayer::togglePathAndArtistClicked(){
    if(isShowingArtist_) isShowingArtist_ = false;
    else if(!displayedArtistName_.empty()) isShowingArtist_ = true;
}

void MusicPlayer::recordCurrentProgress(){
    if(formatContext_ == nullptr) return;
    history_.record(displayedFilePath_, musicTimePlayed_, currentMusicTotalLength_);
}

void MusicPlayer::resumeFromHistory(){
    std::optional<float> savedProgress{history_.findProgress(displayedFilePath_, currentMusicTotalLength_)};
    if(savedProgress) seekToSeconds(savedProgress.value());
}