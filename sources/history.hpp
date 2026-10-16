#pragma once

#include <string>
#include <filesystem>
#include <optional>
#include <list>
#include <cstdint>
#include <map>

#include "constants.hpp"

class PlaybackHistory{
private:
    struct FileIdentity{
        uint64_t fileSize;
        uint64_t fingerprint;

        auto operator<=>(const FileIdentity &) const = default;
    };

    struct Entry{
        FileIdentity identity;
        float progress; // seconds
    };

private:
    std::list<Entry> entries_;
    std::map<FileIdentity, std::list<Entry>::iterator> entryByIdentity_;

    bool isEnabled_{true};
    size_t maxEntries_{constants::history::MaxEntries};
    bool isDirty_{false};
    std::filesystem::path filePath_;

public:
    void setEnabled(bool isEnabled){ isEnabled_ = isEnabled;}
    void setMaxEntries(size_t maxEntries);

    void load();
    void save();

    void record(const std::string &path, float progress, float length);
    std::optional<float> findProgress(const std::string &path, float length) const;

    static bool IsProgressEligible(float progress, float length);

private:
    static std::optional<FileIdentity> ReadFileIdentity(const std::string &path);

    void remove(const FileIdentity &identity);
    void trimToMaxEntries();
};
