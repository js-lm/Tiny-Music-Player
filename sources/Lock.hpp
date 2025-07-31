#pragma once

#include <raylib.h>

#include <optional>
#include <string>
#include <ctime>
#include <fstream>
#include <iostream>
#include <cstdio>
#include <filesystem>

#include "Constants.hpp"

namespace Lock{

    namespace _{

        inline std::time_t GetCurrentTimestamp(){
            return std::time(nullptr);
        }

        inline std::string GetConfigDirectory(){
            std::filesystem::path lockPath;

#if defined(__linux__)
            // Linux: ~/.config
            const char *xdgRuntimeDirectory{std::getenv("XDG_RUNTIME_DIR")};
            if(xdgRuntimeDirectory && xdgRuntimeDirectory[0] != '\0'){
                lockPath = xdgRuntimeDirectory;
            }else{
                const char *tmpDirectory{std::getenv("TMPDIR")};
                if(tmpDirectory && tmpDirectory[0] != '\0'){
                    lockPath = tmpDirectory;
                }else{
                    lockPath = "/tmp";
                }
            }
#elif defined(__APPLE__)

#elif defined(_WIN32)

#endif
            lockPath /= Constants::System::WindowName;

            try{
                std::filesystem::create_directories(lockPath);
            }catch(...){ 
                return "";
            }

            return lockPath.string();
        }
    } // namespace _

    static const std::string LockLocation{_::GetConfigDirectory() + Constants::LockFileName};

    inline void LockProgram(){
        SaveFileText(LockLocation.c_str(), const_cast<char*>(std::to_string(_::GetCurrentTimestamp()).c_str()));
    }

    inline void UpdateLockTimeStamp(){
        LockProgram();
    }

    inline void UnlockProgram(){
        if(FileExists(LockLocation.c_str())){
            std::remove(LockLocation.c_str());
        }
    }

    inline bool IsProgramLocked(){
        if(!FileExists(LockLocation.c_str())) return false;

        std::ifstream lock(LockLocation);
        std::string content;

        bool isLocked{true};

        if(std::getline(lock, content)){
            long long lastUpdateTime{0};

            try{
                lastUpdateTime = std::stoll(content);
            }catch(...){}

            if(_::GetCurrentTimestamp() - static_cast<std::time_t>(lastUpdateTime) >= Constants::LockExpirationTime){
                UnlockProgram();
                isLocked = false;
            }
        }
        
        lock.close();

        return isLocked;
    }

    inline void WriteNewFilePath(const std::string &path){
        if(!IsProgramLocked()) return;

        std::string oldTimestamp;
        /* Get old timestamp */ {
            std::ifstream lock(LockLocation);
            std::string content;
            if(std::getline(lock, content)) oldTimestamp = content;
            lock.close();
        } /* Get old timestamp */

        std::ofstream lock(LockLocation);
        lock << oldTimestamp << "\n" << path << "";
        lock.close();
    }

    inline std::optional<std::string> TryGetNewFilePath(){
        if(!FileExists(LockLocation.c_str())) return std::nullopt;

        std::ifstream lock(LockLocation);
        std::string content;

        constexpr int targetLine{2}; // 1 based
        for(int line{1}; line <= targetLine && std::getline(lock, content); line++){
            if(line == targetLine){
                return content;
            }
        }

        lock.close();

        return std::nullopt;
    }

} // namespace Lock