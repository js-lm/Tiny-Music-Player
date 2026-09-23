#pragma once

#include <raylib.h>

#include <optional>
#include <string>
#include <ctime>
#include <fstream>
#include <iostream>
#include <cstdio>
#include <filesystem>

#include "constants.hpp"

namespace lock{

    namespace _{

        // inline std::time_t GetCurrentTimestamp(){
        //     return std::time(nullptr);
        // }

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
            lockPath /= constants::system::WindowName;

            try{
                std::filesystem::create_directories(lockPath);
            }catch(...){ 
                return "";
            }

            return lockPath.string();
        }
    } // namespace _

    static const std::string LockLocation{_::GetConfigDirectory() + constants::LockFileName};
    static const std::string IpcLocation{_::GetConfigDirectory() + constants::IpcFileName};
    
#if defined(__linux__)
    #include <sys/file.h>
    #include <fcntl.h>
    #include <unistd.h>
    static int lockFd{-1};
#endif

    inline bool TryAcquireLock(){
#if defined(__linux__)
        lockFd = open(LockLocation.c_str(), O_CREAT | O_RDWR, 0666);
        if(lockFd == -1) return false;
        
        if(flock(lockFd, LOCK_EX | LOCK_NB) == -1){
            close(lockFd);
            lockFd = -1;
            return false;
        }
        return true;
#elif defined(__APPLE__)
        return true;
#elif defined(_WIN32)
        return true;
#else
        return true;
#endif
    }

    inline void UnlockProgram(){
#if defined(__linux__)
        if(lockFd != -1){
            flock(lockFd, LOCK_UN);
            close(lockFd);
            lockFd = -1;
            std::remove(LockLocation.c_str());
        }
#endif
    }

    inline void WriteNewFilePath(const std::string &path){
        std::ofstream ipc(IpcLocation, std::ios::app);
        if(ipc.is_open()){
            ipc << path << "\n";
        }
    }

    inline std::optional<std::string> TryGetNewFilePath(){
        if(!FileExists(IpcLocation.c_str())) return std::nullopt;

        std::ifstream ipcIn(IpcLocation);
        if(!ipcIn.is_open()) return std::nullopt;

        std::string firstLine;
        bool hasLine{static_cast<bool>(std::getline(ipcIn, firstLine))};
        
        std::vector<std::string> remainingLines;
        std::string line;
        while(std::getline(ipcIn, line)){
            remainingLines.push_back(line);
        }
        ipcIn.close();

        if(!hasLine || firstLine.empty()){
            std::remove(IpcLocation.c_str());
            return std::nullopt;
        }

        if(remainingLines.empty()){
            std::remove(IpcLocation.c_str());
        }else{
            std::ofstream ipcOut(IpcLocation, std::ios::trunc);
            for(const auto &remainingLine : remainingLines){
                ipcOut << remainingLine << "\n";
            }
        }

        return firstLine;
    }

} // namespace lock