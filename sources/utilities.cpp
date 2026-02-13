#include "music_player.hpp"

#include "constants.hpp"

#include "lock.hpp"

#include "debug_utilities.hpp"

#include <sstream>
#include <iomanip>
#include <random>
#include <algorithm>
#include <iostream>
#include <random>
#include <algorithm>
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
        const std::bitset<Constants::Icons::NumberOfColumns> &bitsetRow{Constants::Icons::IconsBitset[row]};
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
    
    // if(!pendingNextMusicPath_.empty()){
    //     std::string nextPath{pendingNextMusicPath_};
    //     pendingNextMusicPath_.clear();
    //     tryStartMusicStream(nextPath.c_str());
    // }
    
    if(formatContext_ == nullptr) return;

    // UpdateMusicStream(music_);

    float musicTimePlayed{musicTimePlayed_};
    
    // musicProgress_ = musicTimePlayed / currentMusicTotalLength_;
    // currentProgressString_ = secondInFloatToString(musicTimePlayed);
    if(!isCurrentlyInteractingWithProgressBar_){
        DEBUG_PRINT_IF_CHANGED("[updateMusic] overwriting musicProgress_={:.4f} with musicTimePlayed={:.4f}", musicProgress_, musicTimePlayed);
        musicProgress_ = musicTimePlayed / currentMusicTotalLength_;
        currentProgressString_ = secondInFloatToString(musicTimePlayed);
    }else{
        currentProgressString_ = secondInFloatToString(musicProgress_ * currentMusicTotalLength_);
    }

    if(!IsAudioStreamPlaying(audioStream_) && !isManuallyPaused_ && !isCurrentlyInteractingWithProgressBar_){
        DEBUG_PRINT("[updateMusic] handleMusicEnd triggered! isPlaying={} isManuallyPaused={} isInteracting={}", IsAudioStreamPlaying(audioStream_), isManuallyPaused_, isCurrentlyInteractingWithProgressBar_);
        handleMusicEnd();
    }
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

void MusicPlayer::initMusicStream(const char *path){
    if(!FileExists(path) && !DirectoryExists(path)) return;
    
    unloadDirectory();
    
    std::string directoryPath{path};
    if(IsPathFile(path)){
        directoryPath = GetDirectoryPath(path);
        currentFileName_ = GetFileName(path);
    }else{
        currentFileName_ = "";
    }
    
    currentDirectoryPath_ = directoryPath;
    
    totalFilesInDirectory_ = 0;
    try{
        for(const auto &entry : std::filesystem::directory_iterator(currentDirectoryPath_)){
            if(entry.is_regular_file()) totalFilesInDirectory_++;
        }
    }catch(...){}
    
    if(!currentFileName_.empty()){
        tryStartMusicStream(path);
    }else{
        findNextValidMusic(true);
    }
}

void MusicPlayer::unloadDirectory(){
    currentDirectoryPath_.clear();
    currentFileName_.clear();
    playedFiles_.clear();
    totalFilesInDirectory_ = 0;
    resetMusicState();
}

bool MusicPlayer::isMediaFile(const std::string &path){
    auto dotPosition{path.rfind('.')};
    if(dotPosition != std::string::npos){
        std::string extension{path.substr(dotPosition)};
        std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

        for(const auto &audioExtension : Constants::SupportedMusicExtensions){
            if(extension == audioExtension) return true;
        }
    }

    // fallback
    AVFormatContext *formatContext{avformat_alloc_context()};
    if(!formatContext) return false;

    if(avformat_open_input(&formatContext, path.c_str(), nullptr, nullptr) == 0){
        bool hasAudioStream{false};
        if(avformat_find_stream_info(formatContext, nullptr) >= 0){
            for(unsigned int i{0}; i < formatContext->nb_streams; i++){
                if(formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO){
                    hasAudioStream = true;
                    break;
                }
            }
        }
        avformat_close_input(&formatContext);
        return hasAudioStream;
    }
    
    if(formatContext) avformat_free_context(formatContext);
    return false;
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
    currentFileName_ = GetFileName(filename);

    return true;
}

void MusicPlayer::findNextValidMusic(bool isForward){
    if(currentDirectoryPath_.empty()) return;

    std::vector<std::string> files;
    try{
        for(const auto &entry : std::filesystem::directory_iterator(currentDirectoryPath_)){
            if(entry.is_regular_file()) files.push_back(entry.path().filename().string());
        }
    } catch(...){ return;}

    if(files.empty()) return;

    std::vector<std::string> mediaFiles;
    for(const auto &file : files){
        if(isMediaFile(file)) mediaFiles.push_back(file);
    }

    if(mediaFiles.empty()) return;

    if(isShuffling_){
        if(!currentFileName_.empty()) playedFiles_.insert(currentFileName_);

        std::vector<std::string> unplayedFiles;
        for(const auto &file : mediaFiles){
            if(playedFiles_.find(file) == playedFiles_.end()){
                unplayedFiles.push_back(file);
            }
        }

        if(unplayedFiles.empty()){
            playedFiles_.clear();
            if(!currentFileName_.empty()) playedFiles_.insert(currentFileName_);
            unplayedFiles = mediaFiles;
        }

        std::random_device randomDevice;
        std::mt19937 generator{randomDevice()};
        std::shuffle(unplayedFiles.begin(), unplayedFiles.end(), generator);

        for(const auto &candidate : unplayedFiles){
            std::string fullPath{currentDirectoryPath_ + "/" + candidate};
            if(tryStartMusicStream(fullPath.c_str())){
                playedFiles_.insert(candidate);
                return;
            }
        }
    }else{
        std::sort(mediaFiles.begin(), mediaFiles.end(), [](const std::string &a, const std::string &b){
            std::string aLower{a};
            std::string bLower{b};
            std::transform(aLower.begin(), aLower.end(), aLower.begin(), ::tolower);
            std::transform(bLower.begin(), bLower.end(), bLower.begin(), ::tolower);
            return aLower < bLower;
        });

        int totalMediaFiles{static_cast<int>(mediaFiles.size())};

        int currentIndex{0};
        for(int i{0}; i < totalMediaFiles; i++){
            if(mediaFiles[i] == currentFileName_){
                currentIndex = i;
                break;
            }
        }

        for(int i{1}; i <= totalMediaFiles; i++){
            int nextIndex{(currentIndex + (isForward ? i : -i) + totalMediaFiles) % totalMediaFiles};
            std::string fullPath{currentDirectoryPath_ + "/" + mediaFiles[nextIndex]};
            if(tryStartMusicStream(fullPath.c_str())) return;
        }
    }
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
