#pragma once

#include <raylib.h>
#include <imgui.h>

#include <string>
#include <optional>
#include <vector>

#include <Constants.hpp>

class MusicPlayer{
private:
    Music music_{};
    float musicProgress_;

    float currentMusicTotalLength_;

    std::string totalLengthString_;
    std::string currentProgressString_;

    std::string displayedMusicTitle_;

    bool isShowingArtist_;
    std::string displayedArtistName_;
    std::string displayedFilePath_;

private:
    Texture2D iconsTexture_;

private:
    std::string programArgumentPath_;

private:
    Constants::LoopMode loopMode_{Constants::LoopMode::No_Loop};

    
    bool isShuffling_{false};
    std::vector<int> shuffleList_;

private:
    FilePathList musicDirectory_{};
    std::optional<int> currentDirectoryIndex_;
    std::optional<int> startingIndex_;

private:
    bool shouldClose_{false};

private: // window dragging event
    bool isDragging_{false};
    Vector2 currentWindowPosition_;
    Vector2 previousMouseScreenPosition_;

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

    std::optional<int> initDirectory(const char *path);
    void unloadDirectory();
    void goToNextMusic();
    void goToPreviousMusic();

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

    bool drawImageButton(Constants::Icons::Id iconId);

    std::string secondInFloatToString(float second);

    bool isExtensionValid(const char *filename);
    bool isMusicFile(const char *filename);

    void shuffleMusic();
};