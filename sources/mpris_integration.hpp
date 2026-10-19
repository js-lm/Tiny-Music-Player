#pragma once

#if defined(__linux__)

#include <string>
#include <memory>
#include <functional>

namespace sdbus{
    class IConnection;
    class IObject;
}

struct MprisCallbacks{
    std::function<void()> onPlayPause;
    std::function<void()> onNext;
    std::function<void()> onPrevious;
    std::function<void()> onStop;
};

class MprisIntegration{
private:
    std::unique_ptr<sdbus::IConnection> connection_;
    std::unique_ptr<sdbus::IObject> object_;

    MprisCallbacks callbacks_;

    bool isPlaying_{false};
    std::string currentTitle_;
    std::string currentArtist_;

    bool canPlayPause_{true};
    bool canGoNext_{true};
    bool canGoPrevious_{true};

public:
    MprisIntegration(MprisCallbacks callbacks);
    ~MprisIntegration();

    void updatePlaybackStatus(bool isPlaying);
    void updateMetadata(const std::string &title, const std::string &artist);
    void updateControls(bool canPlayPause, bool canGoNext, bool canGoPrevious);

private:
    void setupDbus();
    void emitPropertyChange(const std::string &propertyName);
};

#endif // __linux__
