#define RAYGUI_IMPLEMENTATION
#include "music_player.hpp"

#include "constants.hpp"

#include "lock.hpp"

#include "debug_utilities.hpp"

#include <clocale>

extern "C" void glfwPostEmptyEvent(void);

MusicPlayer::MusicPlayer(int argumentCount, char *arguments[]){
    SetTraceLogLevel(LOG_NONE);

    std::optional<std::string> path{getArgumentPath(argumentCount, arguments)};

    if(!lock::TryAcquireLock()){
        if(path) lock::WriteNewFilePath(path.value());
        shouldClose_ = true;
    }else{
        if(path) programArgumentPath_ = path.value();
        // lock::LockProgram();
    }
}

int MusicPlayer::run(){
    if(shouldClose_) return 0;
    initialize();
    while(!(WindowShouldClose() || shouldClose_)){
        update();
        draw();
    }
    shutdown();
    return 0;
}

void MusicPlayer::initialize(){
    std::setlocale(LC_ALL, "C");
    SetConfigFlags(FLAG_WINDOW_UNDECORATED | FLAG_WINDOW_TRANSPARENT | FLAG_WINDOW_ALWAYS_RUN | FLAG_WINDOW_HIGHDPI);
	InitWindow(
        constants::system::WindowWidth, 
        constants::system::WindowHeight, 
        constants::system::WindowName
    );
    SetWindowOpacity(constants::system::WindowOpacity);
	SetTargetFPS(constants::system::WindowFPS);
    
    dpiScale_ = GetWindowScaleDPI().x;

    renderTexture_ = LoadRenderTexture(
        constants::system::WindowWidth,
        constants::system::WindowHeight
    );
    
    renderSourceRectangle_ = Rectangle{
        0, 0,
        static_cast<float>(constants::system::WindowWidth),
        -static_cast<float>(constants::system::WindowHeight)
    };
    renderDestinationRectangle_ = Rectangle{
        0, 0,
        static_cast<float>(constants::system::WindowWidth),
        static_cast<float>(constants::system::WindowHeight)
    };

    SetAudioStreamBufferSizeDefault(constants::system::AudioBufferSize);
    InitAudioDevice();

    initializeIconsTexture();

    reloadFont();

    resetMusicState();

    initializeWindowIcon();

    // if(!programArgumentPath_.empty()) initializeMusicStream(programArgumentPath_.c_str());
    audioThreadRunning_ = true;
    audioThread_ = std::thread([this](){
        while(this->audioThreadRunning_){
            {
                std::lock_guard<std::recursive_mutex> lock{this->musicMutex_};
                // if(IsMusicValid(this->music_) && IsMusicStreamPlaying(this->music_)){
                //     UpdateMusicStream(this->music_);
                // }
                if(this->isAudioStreamInitialized_ && IsAudioStreamPlaying(this->audioStream_)){
                    if(IsAudioStreamProcessed(this->audioStream_)){
                        int framesNeeded{constants::system::AudioBufferSize};
                        int samplesNeeded{framesNeeded * 2};
                        
                        while(this->audioBuffer_.size() < samplesNeeded){
                            AVPacket *packet{av_packet_alloc()};
                            if(av_read_frame(this->formatContext_, packet) == 0){

                                if(packet->stream_index == this->audioStreamIndex_){

                                    avcodec_send_packet(this->codecContext_, packet);

                                    AVFrame *frame{av_frame_alloc()};
                                    
                                    while(avcodec_receive_frame(this->codecContext_, frame) == 0){
                                        uint8_t *output{nullptr};
                                        int outSamples{swr_get_out_samples(this->swrContext_, frame->nb_samples)};
                                        av_samples_alloc(&output, nullptr, 2, outSamples, AV_SAMPLE_FMT_FLT, 0);
                                    
                                        outSamples = swr_convert(this->swrContext_, &output, outSamples, (const uint8_t**)frame->data, frame->nb_samples);
                                        
                                        float *floatOutput{reinterpret_cast<float *>(output)};
                                        for(int i{0}; i < outSamples * 2; i++){
                                            this->audioBuffer_.push_back(floatOutput[i]);
                                        }
                                        
                            
                                        if(packet->pts != AV_NOPTS_VALUE){
                                            float packetTime{static_cast<float>(packet->pts * av_q2d(this->formatContext_->streams[this->audioStreamIndex_]->time_base))};
                                            if(this->audioThreadSeekGeneration_ == this->seekGeneration_){
                                                this->musicTimePlayed_ = packetTime;
                                            }else{
                                                DEBUG_PRINT("[audioThread] SKIPPED PTS update: packetTime={:.4f} (gen {} != {})", packetTime, this->audioThreadSeekGeneration_, this->seekGeneration_);
                                            }
                                        }
                                        
                                        av_freep(&output);
                                    }

                                    av_frame_free(&frame);
                                }
                                av_packet_free(&packet);


                            }else{
                                av_packet_free(&packet);

                                break;

                            }
                        }
                        
                        if(this->audioBuffer_.size() >= samplesNeeded){
                            UpdateAudioStream(this->audioStream_, this->audioBuffer_.data(), framesNeeded);
                            this->audioBuffer_.erase(this->audioBuffer_.begin(), this->audioBuffer_.begin() + samplesNeeded);
                            this->audioThreadSeekGeneration_ = this->seekGeneration_;
                        
                        }else if(!this->audioBuffer_.empty()){
                            UpdateAudioStream(this->audioStream_, this->audioBuffer_.data(), this->audioBuffer_.size() / 2);
                            this->audioBuffer_.clear();
                            StopAudioStream(this->audioStream_);

                        }else{
                            StopAudioStream(this->audioStream_);
                        }
                    }
                }
            }
            static int counter{0};
            if(counter++ >= constants::system::AudioThreadEventPostFrequency){
                glfwPostEmptyEvent();
                counter = 0;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(constants::system::AudioThreadSleepDurationMs));
        }
    });

#if defined(__linux__)
    mprisIntegration_ = std::make_unique<MprisIntegration>(MprisCallbacks{
        .onPlayPause {[this](){ mprisPendingPlayPause_ = true; glfwPostEmptyEvent();}},
        .onNext      {[this](){ mprisPendingNext_      = true; glfwPostEmptyEvent();}},
        .onPrevious  {[this](){ mprisPendingPrevious_  = true; glfwPostEmptyEvent();}},
        .onStop      {[this](){ mprisPendingPlayPause_ = true; glfwPostEmptyEvent();}},
    });
    std::setlocale(LC_ALL, "C");
#endif

    if(!programArgumentPath_.empty()) initializeMusicStream(programArgumentPath_.c_str());
}

void MusicPlayer::update(){
    EnableEventWaiting();

#if defined(__linux__)
    if(mprisPendingPlayPause_.exchange(false))  playPauseMusicClicked();
    if(mprisPendingNext_.exchange(false))       nextSongClicked();
    if(mprisPendingPrevious_.exchange(false))   previousSongClicked();
#endif

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
        renderSourceRectangle_,
        renderDestinationRectangle_,
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
    UnloadFont(customFont_);
    UnloadTexture(iconsTexture_);
    UnloadRenderTexture(renderTexture_);
    CloseWindow();
    lock::UnlockProgram();
}