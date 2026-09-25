#pragma once

#include <raylib.h>
#include <string>
#include <optional>
#include <vector>

#include "constants.hpp"

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

#if defined(__linux__)
#include "mpris_integration.hpp"
#endif

class MusicPlayer{
private:
    std::thread audioThread_;
    std::atomic<bool> audioThreadRunning_{false};
    std::recursive_mutex musicMutex_;

    std::thread eventsThread_;
    std::atomic<bool> eventsThreadRunning_{false};
    float previousProgressUpdateTime_{.0f};

private:
    AVFormatContext *formatContext_{nullptr};
    AVCodecContext *codecContext_{nullptr};
    SwrContext *swrContext_{nullptr};
    int audioStreamIndex_{-1};
    AudioStream audioStream_;
    bool isAudioStreamInitialized_{false};
    std::vector<float> audioBuffer_;
    int seekGeneration_{0};
    int audioThreadSeekGeneration_{0};

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
    Font customFont_{};
    float dpiScale_;
    Rectangle renderSourceRectangle_;
    Rectangle renderDestinationRectangle_;

private:
    std::string programArgumentPath_;

private:
    constants::LoopMode loopMode_{constants::LoopMode::No_Loop};

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


#if defined(__linux__)
    std::unique_ptr<MprisIntegration> mprisIntegration_{nullptr};
    
    std::atomic<bool> mprisPendingPlayPause_{false};
    std::atomic<bool> mprisPendingNext_{false};
    std::atomic<bool> mprisPendingPrevious_{false};
#endif

public:
    MusicPlayer(int argumentCount, char *arguments[]);
    ~MusicPlayer() = default;

    int run();

private:
	void drawInterface();
	bool shouldClose() const{ return shouldClose_;}

private:
    void initialize();
    void update();
    void draw();
    void shutdown();

private:
    std::optional<std::string> getArgumentPath(int argumentCount, char *arguments[]);
    
private:
    void updateMusic();
    void resetMusicState();
    void reloadFont();

    void initializeMusicStream(const char *path);
    void tryUnloadMusic();

    bool tryStartMusicStream(const char *filename);

    void unloadDirectory();
    bool findNextValidMusic(bool isForward = true, bool allowLoop = true);
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
    void initializeIconsTexture();
    void initializeWindowIcon();

    bool drawImageButton(constants::icons::Id iconId, Rectangle bounds);

    std::string secondInFloatToString(float second);

    bool isMediaFile(const std::string &path);

    // Vector2 scaleToDpiVector2(Vector2 values){
    //     return Vector2{
    //         values.x * GetWindowScaleDPI().x, 
    //         values.y * GetWindowScaleDPI().y
    //     };
    // }
    // float scaleToDpiFloat(float value){ return value * GetWindowScaleDPI().x;}
    // int scaleToDpiInt(int value){ return static_cast<int>(value * GetWindowScaleDPI().x);}

};