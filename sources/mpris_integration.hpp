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
public:
    MprisIntegration(MprisCallbacks callbacks);
    ~MprisIntegration();

    void updatePlaybackStatus(bool isPlaying);
    void updateMetadata(const std::string &title, const std::string &artist);

private:
    void setupDbus();
    void emitPropertyChange(const std::string &propertyName);

    std::unique_ptr<sdbus::IConnection> connection_;
    std::unique_ptr<sdbus::IObject> object_;

    MprisCallbacks callbacks_;

    bool isPlaying_{false};
    std::string currentTitle_;
    std::string currentArtist_;
};

#endif // __linux__
