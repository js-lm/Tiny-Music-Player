#include "MusicPlayer.hpp"

#include "MetadataParser.hpp"
#include "Constants.hpp"
#include "Lock.hpp"

#include <sstream>
#include <iomanip>
#include <random>
#include <algorithm>
#include <iostream>

#include <raymath.h>

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
    if(!IsMusicValid(music_)) return;

    UpdateMusicStream(music_);

    auto musicTimePlayed{GetMusicTimePlayed(music_)};
    
    musicProgress_ = musicTimePlayed / currentMusicTotalLength_;
    currentProgressString_ = secondInFloatToString(musicTimePlayed);

    if(!IsMusicStreamPlaying(music_) && !isManuallyPaused_ && !isCurrentlyInteractingWithProgressBar_) handleMusicEnd();    
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
    if(IsMusicValid(music_)){
        StopMusicStream(music_);
        UnloadMusicStream(music_);
        music_ = Music{};
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

    music_ = LoadMusicStream(filename);

    if(IsMusicValid(music_)){
        PlayMusicStream(music_);
        isManuallyPaused_ = false;

        music_.looping = loopMode_ == Constants::LoopMode::Single_Music_Loop;
        
        currentMusicTotalLength_ = GetMusicTimeLength(music_);
        totalLengthString_ = secondInFloatToString(currentMusicTotalLength_);
        currentProgressString_ = secondInFloatToString(.0f);
        
        auto metadata{GetMusicMetadata(filename)};
        displayedMusicTitle_ = metadata.title;
        displayedArtistName_ = metadata.artist;
        displayedFilePath_ = filename;

        return true;
    }

    return false;
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

    auto music{LoadSound(filename)};
    if(IsSoundValid(music)){
        UnloadSound(music);
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
