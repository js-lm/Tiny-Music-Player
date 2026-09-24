#include "mpris_integration.hpp"

#if defined(__linux__)

#include <sdbus-c++/sdbus-c++.h>
#include <iostream>
#include <map>

const std::string MPRIS_NAME{"org.mpris.MediaPlayer2.tinymusicplayer"};
const std::string MPRIS_PATH{"/org/mpris/MediaPlayer2"};
const std::string MPRIS_ROOT_INTERFACE{"org.mpris.MediaPlayer2"};
const std::string MPRIS_PLAYER_INTERFACE{"org.mpris.MediaPlayer2.Player"};

MprisIntegration::MprisIntegration(MprisCallbacks callbacks) 
    : callbacks_(std::move(callbacks)){
    try{
        setupDbus();
    }catch(const sdbus::Error &error){
        std::cerr << "Failed to initialize MPRIS D-Bus: " << error.getName() << " - " << error.getMessage() << std::endl;
    }
}

MprisIntegration::~MprisIntegration(){
    if(connection_) connection_->leaveEventLoop();
}

void MprisIntegration::emitPropertyChange(const std::string &propertyName){
    if(!object_) return;

    try{
        object_->emitPropertiesChangedSignal(
            sdbus::InterfaceName{MPRIS_PLAYER_INTERFACE},
            {sdbus::PropertyName{propertyName}}
        );
    }catch(const sdbus::Error &error){
        std::cerr << "Failed to emit " << propertyName << " changed: " << error.getMessage() << std::endl;
    }
}

void MprisIntegration::updatePlaybackStatus(bool isPlaying){
    isPlaying_ = isPlaying;
    emitPropertyChange("PlaybackStatus");
}

void MprisIntegration::updateMetadata(const std::string &title, const std::string &artist){
    currentTitle_ = title;
    currentArtist_ = artist;
    emitPropertyChange("Metadata");
}

void MprisIntegration::setupDbus(){
    connection_ = sdbus::createSessionBusConnection(sdbus::ServiceName{MPRIS_NAME});
    object_ = sdbus::createObject(*connection_, sdbus::ObjectPath{MPRIS_PATH});

    // org.mpris.MediaPlayer2
    object_->addVTable(sdbus::InterfaceName{MPRIS_ROOT_INTERFACE},
        sdbus::registerProperty("CanQuit").withGetter([](){ return false;}),
        sdbus::registerProperty("CanRaise").withGetter([](){ return false;}),
        sdbus::registerProperty("HasTrackList").withGetter([](){ return false;}),
        sdbus::registerProperty("Identity").withGetter([](){ return "Tiny Music Player";}),
        sdbus::registerProperty("DesktopEntry").withGetter([](){ return "tinymusicplayer";}),
        sdbus::registerProperty("SupportedUriSchemes").withGetter([](){ return std::vector<std::string>{"file"};}),
        sdbus::registerProperty("SupportedMimeTypes").withGetter([](){ return std::vector<std::string>{"audio/mpeg", "audio/flac", "audio/x-wav"};}),
        sdbus::registerMethod("Raise").implementedAs([](){}),
        sdbus::registerMethod("Quit").implementedAs([](){})
    );

    // org.mpris.MediaPlayer2.Player
    object_->addVTable(sdbus::InterfaceName{MPRIS_PLAYER_INTERFACE},
        sdbus::registerMethod("PlayPause").implementedAs([this](){ if(callbacks_.onPlayPause) callbacks_.onPlayPause();}),
        sdbus::registerMethod("Play").implementedAs([this](){ if(callbacks_.onPlayPause) callbacks_.onPlayPause();}),
        sdbus::registerMethod("Pause").implementedAs([this](){ if(callbacks_.onPlayPause) callbacks_.onPlayPause();}),
        sdbus::registerMethod("Stop").implementedAs([this](){ if(callbacks_.onStop) callbacks_.onStop();}),
        sdbus::registerMethod("Next").implementedAs([this](){ if(callbacks_.onNext) callbacks_.onNext();}),
        sdbus::registerMethod("Previous").implementedAs([this](){ if(callbacks_.onPrevious) callbacks_.onPrevious();}),

        sdbus::registerProperty("PlaybackStatus").withGetter([this](){
            return isPlaying_ ? "Playing" : "Paused";
        }),
        sdbus::registerProperty("Metadata").withGetter([this](){
            std::map<std::string, sdbus::Variant> metadata;
            metadata["mpris:trackid"] = sdbus::Variant(sdbus::ObjectPath("/org/mpris/MediaPlayer2/TrackList/NoTrack"));
            if(!currentTitle_.empty()) metadata["xesam:title"] = sdbus::Variant(currentTitle_);
            if(!currentArtist_.empty()) metadata["xesam:artist"] = sdbus::Variant(std::vector<std::string>{currentArtist_});
            return metadata;
        }),
        sdbus::registerProperty("LoopStatus").withGetter([](){ return "None";}),
        sdbus::registerProperty("Rate").withGetter([](){ return 1.0;}),
        sdbus::registerProperty("Shuffle").withGetter([](){ return false;}),
        sdbus::registerProperty("Volume").withGetter([](){ return 1.0;}),
        sdbus::registerProperty("Position").withGetter([]()->int64_t{ return 0;}),
        sdbus::registerProperty("MinimumRate").withGetter([](){ return 1.0;}),
        sdbus::registerProperty("MaximumRate").withGetter([](){ return 1.0;}),
        sdbus::registerProperty("CanGoNext").withGetter([](){ return true;}),
        sdbus::registerProperty("CanGoPrevious").withGetter([](){ return true;}),
        sdbus::registerProperty("CanPlay").withGetter([](){ return true;}),
        sdbus::registerProperty("CanPause").withGetter([](){ return true;}),
        sdbus::registerProperty("CanSeek").withGetter([](){ return false;}),
        sdbus::registerProperty("CanControl").withGetter([](){ return true;})
    );
    connection_->enterEventLoopAsync();
}

#endif // __linux__
