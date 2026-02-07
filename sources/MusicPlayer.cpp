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
                // if(IsMusicValid(this->music_) && IsMusicStreamPlaying(this->music_)){
                //     UpdateMusicStream(this->music_);
                // }
                if(this->isAudioStreamInitialized_ && IsAudioStreamPlaying(this->audioStream_)){
                    if(IsAudioStreamProcessed(this->audioStream_)){
                        int framesNeeded{Constants::System::AudioBufferSize};
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
                                            this->musicTimePlayed_ = static_cast<float>(packet->pts) * av_q2d(this->formatContext_->streams[this->audioStreamIndex_]->time_base);
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