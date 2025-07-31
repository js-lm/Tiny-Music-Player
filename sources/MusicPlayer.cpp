#include "MusicPlayer.hpp"

#include "Constants.hpp"
#include "Lock.hpp"

#include <imgui.h> 
#include <rlImGui.h>

MusicPlayer::MusicPlayer(int argumentCount, char *arguments[]){
    SetTraceLogLevel(LOG_NONE);

    std::optional<std::string> path{getArgumentPath(argumentCount, arguments)};

    if(Lock::IsProgramLocked()){
        if(path) Lock::WriteNewFilePath(path.value());
        shouldClose_ = true;
    }else{
        if(path) programArgumentPath_ = path.value();
        Lock::LockProgram();
    }
}

int MusicPlayer::run(){
    if(shouldClose_) return 0;
    init();
    while(!(WindowShouldClose() || shouldClose_)){
        update();
        draw();
    }
    shutdown();
    return 0;
}

void MusicPlayer::init(){
	SetConfigFlags(FLAG_WINDOW_UNDECORATED | FLAG_WINDOW_TRANSPARENT | FLAG_WINDOW_ALWAYS_RUN);
	InitWindow(
        Constants::System::WindowWidth, 
        Constants::System::WindowHeight, 
        Constants::System::WindowName
    );
    SetWindowOpacity(Constants::System::WindowOpacity);
	SetTargetFPS(Constants::System::WindowFPS);
    SetWindowSize(
        scaleToDpiInt(Constants::System::WindowWidth),
        scaleToDpiInt(Constants::System::WindowHeight)
    );

	rlImGuiSetup(false);
    ImGuiIO &io{ImGui::GetIO()};
	io.ConfigWindowsMoveFromTitleBarOnly = true;
    io.IniFilename = nullptr;
    ImGui::GetStyle().ScaleAllSizes(scaleToDpiFloat(1));

    SetAudioStreamBufferSizeDefault(Constants::System::AudioBufferSize);
    InitAudioDevice();

    initIconsTexture();

    resetMusicState();

    initWindowIcon();

    if(!programArgumentPath_.empty()) initMusicStream(programArgumentPath_.c_str());
}

void MusicPlayer::update(){
    handleNewInstanceOpened();
    updateMusic();

    handleKeyboard();
    handleWindowDrag();
    handleFileDrop();
}

void MusicPlayer::draw(){
    BeginDrawing();
    ClearBackground(BLANK);

    rlImGuiBegin();
    drawInterface();
    rlImGuiEnd();

    EndDrawing();
}

void MusicPlayer::shutdown(){
    tryUnloadMusic();
    CloseAudioDevice();
    UnloadTexture(iconsTexture_);
    CloseWindow();
    Lock::UnlockProgram();
}