#define RAYGUI_IMPLEMENTATION
#include "MusicPlayer.hpp"

#include "Constants.hpp"
#include "Lock.hpp"

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
    
    dpiScale_ = GetWindowScaleDPI().x;
    
    SetWindowSize(
        scaleToDpiInt(Constants::System::WindowWidth),
        scaleToDpiInt(Constants::System::WindowHeight)
    );

    renderTexture_ = LoadRenderTexture(
        Constants::System::WindowWidth,
        Constants::System::WindowHeight
    );
    
    renderSourceRect_ = Rectangle{
        0, 0,
        static_cast<float>(Constants::System::WindowWidth),
        -static_cast<float>(Constants::System::WindowHeight)
    };
    renderDestRect_ = Rectangle{
        0, 0,
        static_cast<float>(scaleToDpiInt(Constants::System::WindowWidth)),
        static_cast<float>(scaleToDpiInt(Constants::System::WindowHeight))
    };

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
    BeginTextureMode(renderTexture_);
    ClearBackground(BLANK);
    
    drawInterface();
    
    EndTextureMode();
    
    BeginDrawing();
    ClearBackground(BLANK);
    
    DrawTexturePro(
        renderTexture_.texture,
        renderSourceRect_,
        renderDestRect_,
        Vector2{0, 0},
        .0f,
        WHITE
    );
    
    EndDrawing();
}

void MusicPlayer::shutdown(){
    tryUnloadMusic();
    CloseAudioDevice();
    UnloadTexture(iconsTexture_);
    UnloadRenderTexture(renderTexture_);
    CloseWindow();
    Lock::UnlockProgram();
}