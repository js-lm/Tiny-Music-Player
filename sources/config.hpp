#pragma once

#include <cstddef>
#include <cstdlib>
#include <filesystem>

#include "constants.hpp"

struct Config{
    bool historyEnabled{true};
    size_t historyMaxEntries{constants::history::MaxEntries};
};

namespace config{
    inline std::filesystem::path GetApplicationConfigDirectory(){
#if defined(__linux__)
        const char *xdgConfigHome{std::getenv("XDG_CONFIG_HOME")};
        if(xdgConfigHome && xdgConfigHome[0] != '\0'){
            std::filesystem::path configHome{xdgConfigHome};
            if(configHome.is_absolute()) return configHome / constants::system::ConfigDirectoryName;
        }

        const char *homeDirectory{std::getenv("HOME")};
        if(homeDirectory && homeDirectory[0] != '\0'){
            return std::filesystem::path{homeDirectory} / ".config" / constants::system::ConfigDirectoryName;
        }
#endif

        return {};
        
    }

} // namespace config
