#include "history.hpp"

#include "config.hpp"
#include "constants.hpp"

#include <array>
#include <fstream>
#include <system_error>

// 64-bit FNV-1a
static uint64_t HashBytes(const char *data, size_t length){
    uint64_t hash{14695981039346656037ull};
    for(size_t index{0}; index < length; index++){
        hash ^= static_cast<unsigned char>(data[index]);
        hash *= 1099511628211ull;
    }
    return hash;
}

std::optional<PlaybackHistory::FileIdentity> PlaybackHistory::ReadFileIdentity(const std::string &path){
    std::error_code errorCode;
    uintmax_t fileSize{std::filesystem::file_size(path, errorCode)};
    if(errorCode) return std::nullopt;

    std::ifstream file{path, std::ios::binary};
    if(!file.is_open()) return std::nullopt;

    std::array<char, constants::history::FingerprintBytes> buffer{};
    file.read(buffer.data(), buffer.size());

    return FileIdentity{
        fileSize,
        HashBytes(buffer.data(), static_cast<size_t>(file.gcount()))
    };
}

void PlaybackHistory::setMaxEntries(size_t maxEntries){
    maxEntries_ = maxEntries < 1 ? 1 : maxEntries;
    trimToMaxEntries();
}

void PlaybackHistory::load(){
    entries_.clear();
    entryByIdentity_.clear();
    isDirty_ = false;

    std::filesystem::path directory{config::GetApplicationConfigDirectory()};
    if(directory.empty()){
        filePath_.clear();
        return;
    }
    filePath_ = directory / constants::history::FileName;

    std::ifstream file{filePath_};
    if(!file.is_open()) return;

    std::string line;
    if(!std::getline(file, line) || line != std::to_string(constants::history::FileVersion)) return;

    // size \t fingerprint \t progress (most recent first)
    while(std::getline(file, line) && entries_.size() < maxEntries_){
        size_t firstTab{line.find('\t')};
        size_t secondTab{firstTab == std::string::npos ? firstTab : line.find('\t', firstTab + 1)};
        if(secondTab == std::string::npos) continue;

        try{
            FileIdentity identity{
                std::stoull(line.substr(0, firstTab)),
                std::stoull(line.substr(firstTab + 1, secondTab - firstTab - 1))
            };
            if(entryByIdentity_.contains(identity)) continue;

            entries_.push_back(Entry{identity, std::stof(line.substr(secondTab + 1))});
            entryByIdentity_[identity] = std::prev(entries_.end());
        }catch(...){
            continue;
        }
    }
}

void PlaybackHistory::save(){
    if(!isDirty_ || filePath_.empty()) return;

    std::error_code errorCode;
    std::filesystem::create_directories(filePath_.parent_path(), errorCode);
    if(errorCode) return;

    std::filesystem::path temporaryPath{filePath_};
    temporaryPath += ".tmp";

    {
        std::ofstream file{temporaryPath, std::ios::trunc};
        if(!file.is_open()) return;

        file << constants::history::FileVersion << '\n';
        for(const Entry &entry : entries_){
            file << entry.identity.fileSize << '\t' 
                 << entry.identity.fingerprint << '\t' 
                 << entry.progress << '\n';
        }
        if(!file) return;
    }

    std::filesystem::rename(temporaryPath, filePath_, errorCode);
    if(!errorCode) isDirty_ = false;
}

void PlaybackHistory::record(const std::string &path, float progress, float length){
    if(!isEnabled_) return;

    std::optional<FileIdentity> identity{ReadFileIdentity(path)};
    if(!identity) return;

    if(!IsProgressEligible(progress, length)){
        remove(identity.value());
        return;
    }

    auto found{entryByIdentity_.find(identity.value())};
    if(found != entryByIdentity_.end()){
        auto entryIterator{found->second};
        if(entryIterator == entries_.begin() && entryIterator->progress == progress) return;

        entryIterator->progress = progress;
        entries_.splice(entries_.begin(), entries_, entryIterator);
    }else{
        entries_.push_front(Entry{identity.value(), progress});
        entryByIdentity_[identity.value()] = entries_.begin();
        trimToMaxEntries();
    }

    isDirty_ = true;
}

std::optional<float> PlaybackHistory::findProgress(const std::string &path, float length) const{
    if(!isEnabled_) return std::nullopt;

    std::optional<FileIdentity> identity{ReadFileIdentity(path)};
    if(!identity) return std::nullopt;

    auto found{entryByIdentity_.find(identity.value())};
    if(found == entryByIdentity_.end()) return std::nullopt;

    float progress{found->second->progress};
    if(progress <= .0f || !(progress < length)) return std::nullopt;
    return progress;
}

bool PlaybackHistory::IsProgressEligible(float progress, float length){
    using namespace constants::history;

    if(!(length >= MinimumTrackedLength)) return false;

    float head{ShortTrackHead};
    float tail{ShortTrackTail};
    if(length >= LongTrackBoundary){
        head = LongTrackHead;
        tail = LongTrackTail;
    }else if(length >= ShortTrackBoundary){
        float ratio{(length - ShortTrackBoundary) / (LongTrackBoundary - ShortTrackBoundary)};
        head = ShortTrackHead + (LongTrackHead - ShortTrackHead) * ratio;
        tail = ShortTrackTail + (LongTrackTail - ShortTrackTail) * ratio;
    }

    return progress >= head && progress <= length - tail;
}

void PlaybackHistory::remove(const FileIdentity &identity){
    auto found{entryByIdentity_.find(identity)};
    if(found == entryByIdentity_.end()) return;

    entries_.erase(found->second);
    entryByIdentity_.erase(found);
    isDirty_ = true;
}

void PlaybackHistory::trimToMaxEntries(){
    while(entries_.size() > maxEntries_){
        entryByIdentity_.erase(entries_.back().identity);
        entries_.pop_back();
        isDirty_ = true;
    }
}
