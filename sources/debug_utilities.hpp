#pragma once

#include <string>
#include <utility>

#include <fmt/format.h>
#include <raylib.h>

#ifdef DEBUG
    #define DEBUG_PRINT(formatString, ...) \
        do{ \
            fmt::print("{}\n", fmt::format(formatString __VA_OPT__(,) __VA_ARGS__)); \
        } while(false)

    #define DEBUG_PRINT_IF_CHANGED(formatString, ...) \
        do{ \
            static std::string previousMessage{}; \
            std::string currentMessage{fmt::format(formatString __VA_OPT__(,) __VA_ARGS__)}; \
            if(currentMessage != previousMessage){ \
                fmt::print("{}\n", currentMessage); \
                previousMessage = std::move(currentMessage); \
            } \
        } while(false)

    #define DEBUG_PRINT_TIMED(intervalMs, formatString, ...) \
        do{ \
            static double previousPrintTimeSeconds{.0}; \
            double currentTimeSeconds{GetTime()}; \
            if((currentTimeSeconds - previousPrintTimeSeconds) * 1000.0 >= static_cast<double>(intervalMs)){ \
                fmt::print("{}\n", fmt::format(formatString __VA_OPT__(,) __VA_ARGS__)); \
                previousPrintTimeSeconds = currentTimeSeconds; \
            } \
        } while(false)

    #define DEBUG_SLEEP_MS(durationMs) \
        do{ \
            WaitTime(static_cast<double>(durationMs) / 1000.0); \
        } while(false)
#else
    #define DEBUG_PRINT(formatString, ...) do{} while(false)
    #define DEBUG_PRINT_IF_CHANGED(formatString, ...) do{} while(false)
    #define DEBUG_PRINT_TIMED(intervalMs, formatString, ...) do{} while(false)
    #define DEBUG_SLEEP_MS(durationMs) do{} while(false)
#endif