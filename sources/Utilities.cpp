#include "MusicPlayer.hpp"

#include "Constants.hpp"
#include "Lock.hpp"

#include <sstream>
#include <iomanip>
#include <random>
#include <algorithm>
#include <iostream>
#include <cstring>

#include <raymath.h>

bool MusicPlayer::drawImageButton(Constants::Icons::Id iconId, Rectangle bounds){
    Vector2 mousePosition{GetMousePosition()};
    bool isHovered{CheckCollisionPointRec(mousePosition, bounds)};
    bool isClicked{false};
    
    if(isHovered){
        isAnyWidgetHovered_ = true;
        if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)){
            isClicked = true;
        }
    }
    
    int offsetYPosition{0};
    if(IsMouseButtonDown(MOUSE_BUTTON_LEFT) && isHovered){
        offsetYPosition = Constants::Icons::IconSize.y * 2; // active
    }else if(isHovered){
        offsetYPosition = Constants::Icons::IconSize.y * 1; // hover
    }
    
    int pixelSize{scaleToDpiInt(1)};
    Rectangle sourceRectangle{
        Constants::Icons::IconSize.x * static_cast<float>(static_cast<int>(iconId)) * pixelSize,
        static_cast<float>(offsetYPosition) * pixelSize,
        Constants::Icons::IconSize.x * pixelSize,
        Constants::Icons::IconSize.y * pixelSize
    };
    
    Rectangle destinationRectangle{
        bounds.x + Constants::Icons::IconOffset.x,
        bounds.y + Constants::Icons::IconOffset.y,
        Constants::Icons::IconSize.x,
        Constants::Icons::IconSize.y
    };
    
    DrawTexturePro(iconsTexture_, sourceRectangle, destinationRectangle, Vector2{0, 0}, .0f, WHITE);
    
    return isClicked;
}

void MusicPlayer::initIconsTexture(){
    Image iconsImage{GenImageColor(
        scaleToDpiInt(Constants::Icons::NumberOfColumns), 
        scaleToDpiInt(Constants::Icons::NumberOfRows * 3), 
        BLANK
    )};

    const int pixelSize{scaleToDpiInt(1)};

    // void ImageDrawRectangle(Image *dst, int posX, int posY, int width, int height, Color color);       // Draw rectangle within an image


    for(size_t row{0}; row < Constants::Icons::NumberOfRows; row++){
        const auto &bitsetRow{Constants::Icons::IconsBitset[row]};
        for(size_t column{0}; column < Constants::Icons::NumberOfColumns; column++){
            if(bitsetRow.test(column)){
                // ImageDrawPixel(&iconsImage, column, row, Constants::Icons::NormalColor);
                ImageDrawRectangle(
                    &iconsImage, 
                    column * pixelSize, 
                    row * pixelSize, 
                    pixelSize, pixelSize, 
                    Constants::Icons::NormalColor
                );
                ImageDrawRectangle(
                    &iconsImage, 
                    column * pixelSize, 
                    row * pixelSize + Constants::Icons::IconSize.y * pixelSize, 
                    pixelSize, pixelSize, 
                    Constants::Icons::HoverColor
                );
                ImageDrawRectangle(
                    &iconsImage, 
                    column * pixelSize, 
                    row * pixelSize + Constants::Icons::IconSize.y * 2 * pixelSize, 
                    pixelSize, pixelSize, 
                    Constants::Icons::ActiveColor
                );
            }
        }
    }

    ImageFlipHorizontal(&iconsImage);
    iconsTexture_ = LoadTextureFromImage(iconsImage);
    UnloadImage(iconsImage);
}

void MusicPlayer::initWindowIcon(){
    SetWindowIcon(Constants::WindowIcon::image);
}

void MusicPlayer::updateMusic(){
    std::lock_guard<std::recursive_mutex> lock{musicMutex_};
    
    // if(!IsMusicValid(music_)) return;
    if(formatContext_ == nullptr) return;

    // UpdateMusicStream(music_);

    auto musicTimePlayed{musicTimePlayed_};
    
    // musicProgress_ = musicTimePlayed / currentMusicTotalLength_;
    // currentProgressString_ = secondInFloatToString(musicTimePlayed);
    if(!isCurrentlyInteractingWithProgressBar_){
        musicProgress_ = musicTimePlayed / currentMusicTotalLength_;
        currentProgressString_ = secondInFloatToString(musicTimePlayed);
    }else{
        currentProgressString_ = secondInFloatToString(musicProgress_ * currentMusicTotalLength_);
    }

    if(!IsAudioStreamPlaying(audioStream_) && !isManuallyPaused_ && !isCurrentlyInteractingWithProgressBar_) handleMusicEnd();    
}

std::string MusicPlayer::secondInFloatToString(float second){
    int hour{static_cast<int>(second / 3600.0f)};
    second -= hour * 3600.0f;

    int minute{static_cast<int>(second / 60.0f)};
    second -= minute * 60.0f;

    std::stringstream stringStream;

    if(hour > 0) stringStream << std::setw(2) << std::setfill('0') << hour << ":";
    stringStream << std::setw(2) << std::setfill('0') << minute << ":";
    stringStream << std::setw(2) << std::setfill('0') << static_cast<int>(second);

    return stringStream.str();
}

void MusicPlayer::resetMusicState(){
    tryUnloadMusic();

    musicProgress_ = .0f;

    totalLengthString_ = "--:--";
    currentProgressString_ = totalLengthString_;

    displayedMusicTitle_ = "N/A";

    isShowingArtist_ = true;
    displayedArtistName_ = displayedMusicTitle_;
    displayedFilePath_ = displayedMusicTitle_;
}

void MusicPlayer::shuffleMusic(){
    if(musicDirectory_.count <= 0) return;

    shuffleList_.clear();

    for(unsigned int i{0}; i < musicDirectory_.count; i++){
        shuffleList_.emplace_back(i);
    }

    std::random_device randomDevice;
    std::mt19937 generator(randomDevice());
    std::shuffle(shuffleList_.begin(), shuffleList_.end(), generator);
}

void MusicPlayer::tryUnloadMusic(){
    // if(IsMusicValid(music_)){
    //     StopMusicStream(music_);
    //     UnloadMusicStream(music_);
    //     music_ = Music{};
    // }
    if(formatContext_ != nullptr){
        if(isAudioStreamInitialized_){
            StopAudioStream(audioStream_);
            UnloadAudioStream(audioStream_);
            isAudioStreamInitialized_ = false;
        }
        if(swrContext_) swr_free(&swrContext_);
        if(codecContext_) avcodec_free_context(&codecContext_);
        if(formatContext_) avformat_close_input(&formatContext_);
        
        musicTimePlayed_ = .0f;
        audioBuffer_.clear();
    }
}

std::optional<int> MusicPlayer::initDirectory(const char *path){
    if(!FileExists(path) && !DirectoryExists(path)) return std::nullopt;

    auto directoryPath{path};
    
    if(IsPathFile(path)){
        if(!isExtensionValid(path)) return std::nullopt;
        
        directoryPath = GetDirectoryPath(path);
    }
    
    unloadDirectory();
    musicDirectory_ = LoadDirectoryFilesEx(
        directoryPath, Constants::SupportedMusicExtensions, false
    );
    if(musicDirectory_.count <= 0) return std::nullopt;
    
    for(int i{0}; i < static_cast<int>(musicDirectory_.count); i++){
        if(strcmp(musicDirectory_.paths[i], path) == 0){
            return i;
        }
    }

    return 0;
}

void MusicPlayer::initMusicStream(const char *path){
    auto index{initDirectory(path)};
    if(!index) return;
    currentDirectoryIndex_ = startingIndex_ = index;
    tryStartMusicStream(musicDirectory_.paths[index.value()]);
}

void MusicPlayer::unloadDirectory(){
    if(musicDirectory_.count <= 0) return;
    UnloadDirectoryFiles(musicDirectory_);
    musicDirectory_ = FilePathList{};
    currentDirectoryIndex_.reset();
    resetMusicState();
    shuffleList_.clear();
}

bool MusicPlayer::tryStartMusicStream(const char *filename){
    resetMusicState();

    formatContext_ = avformat_alloc_context();
    if(avformat_open_input(&formatContext_, filename, nullptr, nullptr) != 0){
        tryUnloadMusic();
        return false;
    }

    if(avformat_find_stream_info(formatContext_, nullptr) < 0){
        tryUnloadMusic();
        return false;
    }

    audioStreamIndex_ = -1;
    for(unsigned int i{0}; i < formatContext_->nb_streams; i++){
        if(formatContext_->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO){
            audioStreamIndex_ = i;
            break;
        }
    }

    if(audioStreamIndex_ == -1){
        tryUnloadMusic();
        return false;
    }

    AVCodecParameters *codecpar{formatContext_->streams[audioStreamIndex_]->codecpar};
    const AVCodec *codec{avcodec_find_decoder(codecpar->codec_id)};
    if(!codec){
        tryUnloadMusic();
        return false;
    }

    codecContext_ = avcodec_alloc_context3(codec);
    if(avcodec_parameters_to_context(codecContext_, codecpar) < 0){
        tryUnloadMusic();
        return false;
    }

    if(avcodec_open2(codecContext_, codec, nullptr) < 0){
        tryUnloadMusic();
        return false;
    }

    swrContext_ = swr_alloc();
    AVChannelLayout outChannelLayout;
    av_channel_layout_default(&outChannelLayout, 2);

    swr_alloc_set_opts2(
        &swrContext_,
        &outChannelLayout,
        AV_SAMPLE_FMT_FLT,
        44100,
        &codecContext_->ch_layout,
        codecContext_->sample_fmt,
        codecContext_->sample_rate,
        0, 
        nullptr
    );

    if(swr_init(swrContext_) < 0){
        tryUnloadMusic();
        return false;
    }

    SetAudioStreamBufferSizeDefault(Constants::System::AudioBufferSize);
    audioStream_ = LoadAudioStream(44100, 32, 2);
    isAudioStreamInitialized_ = true;
    PlayAudioStream(audioStream_);
    
    isManuallyPaused_ = false;

    currentMusicTotalLength_ = static_cast<float>(formatContext_->duration) / AV_TIME_BASE;
    totalLengthString_ = secondInFloatToString(currentMusicTotalLength_);
    currentProgressString_ = secondInFloatToString(.0f);
    
    // Extract metadata
    AVDictionaryEntry *titleEntry{av_dict_get(formatContext_->metadata, "title", nullptr, 0)};
    AVDictionaryEntry *artistEntry{av_dict_get(formatContext_->metadata, "artist", nullptr, 0)};
    
    displayedMusicTitle_ = titleEntry ? titleEntry->value : GetFileName(filename);
    displayedArtistName_ = artistEntry ? artistEntry->value : displayedMusicTitle_;
    displayedFilePath_ = filename;

    return true;
}

void MusicPlayer::goToNextMusic(){
    if(musicDirectory_.count <= 0 || !currentDirectoryIndex_) return;

    bool hasIteratedTheEntireDirectory{false};
    int startIndex{currentDirectoryIndex_.value()};

    char *currentPath;

    do{
        if(++currentDirectoryIndex_.value() >= static_cast<int>(musicDirectory_.count)){
            currentDirectoryIndex_.value() = 0;
        }

        hasIteratedTheEntireDirectory = currentDirectoryIndex_.value() == startIndex;

        if(isShuffling_){
            if(shuffleList_.size() != musicDirectory_.count) shuffleMusic();

            currentPath = musicDirectory_.paths[shuffleList_[currentDirectoryIndex_.value()]];
        }else{
            currentPath = musicDirectory_.paths[currentDirectoryIndex_.value()];
        }
    }while(!isMusicFile(currentPath) && !hasIteratedTheEntireDirectory);

    if(!hasIteratedTheEntireDirectory){
        tryStartMusicStream(currentPath);
    }else{
        unloadDirectory();
    }
}

void MusicPlayer::goToPreviousMusic(){
    if(musicDirectory_.count <= 0 || !currentDirectoryIndex_) return;

    bool hasIteratedTheEntireDirectory{false};
    int startIndex{currentDirectoryIndex_.value()};

    char *currentPath;

    do{
        if(--currentDirectoryIndex_.value() < 0){
            currentDirectoryIndex_.value() = musicDirectory_.count - 1;
        }

        hasIteratedTheEntireDirectory = currentDirectoryIndex_.value() == startIndex;

        currentPath = musicDirectory_.paths[currentDirectoryIndex_.value()];
    }while(!isMusicFile(currentPath) && !hasIteratedTheEntireDirectory);

    if(!hasIteratedTheEntireDirectory){
        tryStartMusicStream(currentPath);
    }else{
        unloadDirectory();
    }
}

bool MusicPlayer::isExtensionValid(const char *filename){
    return IsFileExtension(filename, Constants::SupportedMusicExtensions);
}

bool MusicPlayer::isMusicFile(const char *filename){
    if(!FileExists(filename) || !isExtensionValid(filename)) return false;

    // auto music{LoadSound(filename)};
    // if(IsSoundValid(music)){
    //     UnloadSound(music);
    auto music{LoadMusicStream(filename)};
    if(IsMusicValid(music)){
        UnloadMusicStream(music);
        return true;
    }

    return false;
}

std::optional<std::string> MusicPlayer::getArgumentPath(int argumentCount, char *arguments[]){
    if(argumentCount <= 1) return std::nullopt;
    std::string pathFound;
    for(int i{1}; i < argumentCount; i++){
        if(IsPathFile(arguments[i]) && pathFound.empty()) pathFound = arguments[i];
        else if(strcmp(arguments[i], "--help") == 0 || strcmp(arguments[i], "-h") == 0){
            std::cout << "A tiny music player created by js-lm (me@joshlam.dev)" << std::endl;
        }else if(strcmp(arguments[i], "--version") == 0 || strcmp(arguments[i], "-v") == 0){
            std::cout << "Version " << Constants::System::AppVersion << std::endl;
        }
    }
    
    return pathFound.empty() ? std::nullopt : std::make_optional(pathFound);
}
