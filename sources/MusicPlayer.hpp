#pragma once

#include <raylib.h>
#include <raygui.h>

#include <string>
#include <optional>
#include <vector>

#include <Constants.hpp>

#include <thread>
#include <atomic>
#include <mutex>
#include <vector>
#include <unordered_set>
#include <filesystem>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
}

class MusicPlayer{
private:
    std::thread audioThread_;
    std::atomic<bool> audioThreadRunning_{false};
    std::recursive_mutex musicMutex_;

private:
    AVFormatContext *formatContext_{nullptr};
    AVCodecContext *codecContext_{nullptr};
    SwrContext *swrContext_{nullptr};
    int audioStreamIndex_{-1};
    AudioStream audioStream_;
    bool isAudioStreamInitialized_{false};
    std::vector<float> audioBuffer_;

    float musicProgress_;
    float musicTimePlayed_{.0f};
    float currentMusicTotalLength_;

    std::string totalLengthString_;
    std::string currentProgressString_;

    std::string displayedMusicTitle_;

    bool isShowingArtist_;
    std::string displayedArtistName_;
    std::string displayedFilePath_;

private:
    Texture2D iconsTexture_;
    RenderTexture2D renderTexture_;
    float dpiScale_;
    Rectangle renderSourceRect_;
    Rectangle renderDestRect_;

private:
    std::string programArgumentPath_;

private:
    Constants::LoopMode loopMode_{Constants::LoopMode::No_Loop};

    bool isShuffling_{false};
    std::unordered_set<std::string> playedFiles_;
    size_t totalFilesInDirectory_{0};
    
private:
    std::string currentDirectoryPath_;
    std::string currentFileName_;

private:
    bool shouldClose_{false};

private: // window dragging event
    bool isDragging_{false};
    Vector2 currentWindowPosition_;
    Vector2 previousMouseScreenPosition_;
    bool isAnyWidgetHovered_{false};

private: // progress bar drag event
    bool wasPausing_{false};

private: // music end event
    bool isManuallyPaused_{false};
    bool isCurrentlyInteractingWithProgressBar_{false};

private: // new instance event
    float timeSinceLastLockUpdate_{Constants::LockUpdateFrequency};

public:
    MusicPlayer(int argumentCount, char *arguments[]);
    ~MusicPlayer() = default;

    int run();

private:
	void drawInterface();
	bool shouldClose() const{ return shouldClose_;}

private:
    void init();
    void update();
    void draw();
    void shutdown();

private:
    std::optional<std::string> getArgumentPath(int argumentCount, char *arguments[]);
    
private:
    void updateMusic();
    void resetMusicState();

    void initMusicStream(const char *path);
    void tryUnloadMusic();

    bool tryStartMusicStream(const char *filename);

    void unloadDirectory();
    void findNextValidMusic(bool isForward = true);
    // void goToPreviousMusic();

private:
    void handleWindowDrag();
    void handleFileDrop();
    void handleMusicEnd();
    void handleKeyboard();
    void handleNewInstanceOpened();

private:
    void minimizeClicked();
    void closeClicked();
    void copyMusicTitleClicked();
    void goToFileClicked();
    void toggleShuffleClicked();
    void previousSongClicked();
    void playPauseMusicClicked();
    void nextSongClicked();
    void toggleLoopClicked(bool isForward = true);
    void progressBarClicked();
    void togglePathAndArtistClicked();

private:
    void initIconsTexture();
    void initWindowIcon();

    bool drawImageButton(Constants::Icons::Id iconId, Rectangle bounds);

    std::string secondInFloatToString(float second);

    bool isMediaFile(const std::string& path);

    Vector2 scaleToDpiVector2(Vector2 values){
        return Vector2{
            values.x * GetWindowScaleDPI().x, 
            values.y * GetWindowScaleDPI().y
        };
    }
    float scaleToDpiFloat(float value){ return value * GetWindowScaleDPI().x;}
    int scaleToDpiInt(int value){ return static_cast<int>(value * GetWindowScaleDPI().x);}

};