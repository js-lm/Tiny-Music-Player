#define RAYGUI_IMPLEMENTATION
#include "MusicPlayer.hpp"

#include "Constants.hpp"
#include "Lock.hpp"

extern "C" void glfwPostEmptyEvent(void);

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
        // We ensure UpdateMusicStream is done in audioThread_
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

    SetMouseScale(1.0f / dpiScale_, 1.0f / dpiScale_);

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

    // if(!programArgumentPath_.empty()) initMusicStream(programArgumentPath_.c_str());
    audioThreadRunning_ = true;
    audioThread_ = std::thread([this](){
        while(this->audioThreadRunning_){
            {
                std::lock_guard<std::recursive_mutex> lock{this->musicMutex_};
                if(IsMusicValid(this->music_) && IsMusicStreamPlaying(this->music_)){
                    UpdateMusicStream(this->music_);
                }
            }
            static int counter{0};
            if(counter++ >= Constants::System::AudioThreadEventPostFrequency){
                glfwPostEmptyEvent();
                counter = 0;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(Constants::System::AudioThreadSleepDurationMs));
        }
    });

    if(!programArgumentPath_.empty()) initMusicStream(programArgumentPath_.c_str());
}

void MusicPlayer::update(){
    EnableEventWaiting();

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
    audioThreadRunning_ = false;
    if(audioThread_.joinable()) audioThread_.join();

    tryUnloadMusic();
    CloseAudioDevice();
    UnloadTexture(iconsTexture_);
    UnloadRenderTexture(renderTexture_);
    CloseWindow();
    Lock::UnlockProgram();
}