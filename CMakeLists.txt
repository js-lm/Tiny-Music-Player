cmake_minimum_required(VERSION 3.22)
project(tiny-music-player CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)

##########################################################################################
# Add dependencies with FetchContent
##########################################################################################

function(add_git_dependency libName gitURL gitTag)
    FetchContent_Declare(${libName}
        GIT_REPOSITORY ${gitURL}
        GIT_TAG        ${gitTag}
        GIT_SHALLOW    TRUE
        GIT_PROGRESS   TRUE
    )
    FetchContent_MakeAvailable(${libName})
    target_compile_options(${libName} PRIVATE "-w")
endfunction()

# Add Raylib
set(SUPPORT_MODULE_RSHAPES  OFF CACHE BOOL "Disable rshapes module" FORCE)
set(SUPPORT_MODULE_RTEXT    OFF CACHE BOOL "Disable rtext module" FORCE)
set(SUPPORT_MODULE_RMODELS  OFF CACHE BOOL "Disable 3D models module" FORCE)

set(SUPPORT_FILEFORMAT_FLAC  ON CACHE BOOL "Enable FLAC support" FORCE)

set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE) # don't build the supplied examples
set(BUILD_GAMES    OFF CACHE BOOL "" FORCE) # don't build the supplied example games
add_git_dependency(
    raylib
    https://github.com/raysan5/raylib.git
    5.5
)

FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG docking
)
set(IMGUI_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(imgui)

FetchContent_Declare(
    rlImGui
    GIT_REPOSITORY https://github.com/raylib-extras/rlImGui.git
    GIT_TAG main
)
FetchContent_GetProperties(rlImGui)
if(NOT rlImGui_POPULATED)
    FetchContent_Populate(rlImGui)
endif()

add_library(rlImGui STATIC
    ${rlimgui_SOURCE_DIR}/rlImGui.cpp
    ${imgui_SOURCE_DIR}/imgui.cpp

    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp

    # ${imgui_SOURCE_DIR}/imgui_demo.cpp
)
target_include_directories(rlImGui PUBLIC
    ${rlimgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}
)
target_link_libraries(rlImGui PUBLIC raylib)

##########################################################################################
# Project executable setup
##########################################################################################

file(GLOB_RECURSE PROJECT_SOURCES CONFIGURE_DEPENDS 
    "${CMAKE_CURRENT_LIST_DIR}/sources/*.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/include/*.cpp"
)

set(PROJECT_INCLUDE "${CMAKE_CURRENT_LIST_DIR}/sources/")

add_executable(${PROJECT_NAME})
target_sources(${PROJECT_NAME} PRIVATE ${PROJECT_SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE 
    ${PROJECT_INCLUDE}
    ${CMAKE_CURRENT_SOURCE_DIR}/resources
    include
)

# Find libid3tag (macOS/Linux)
if(APPLE)
    message(STATUS "Configuring for macOS")
    find_library(ID3TAG_LIBRARY NAMES id3tag PATHS /opt/homebrew/lib REQUIRED)
    find_path(ID3TAG_INCLUDE_DIR NAMES id3tag.h PATHS /opt/homebrew/include REQUIRED)
elseif(UNIX)
    message(STATUS "Configuring for Linux")
    find_library(ID3TAG_LIBRARY NAMES id3tag PATHS /usr/lib /usr/local/lib REQUIRED)
    find_path(ID3TAG_INCLUDE_DIR NAMES id3tag.h PATHS /usr/include /usr/local/include REQUIRED)
endif()

# Link and include
target_include_directories(tiny-music-player PRIVATE ${ID3TAG_INCLUDE_DIR})
target_link_libraries(tiny-music-player PRIVATE ${ID3TAG_LIBRARY})


target_link_libraries(${PROJECT_NAME} PRIVATE
    raylib
    rlImGui
)

##########################################################################################
# Project build settings
##########################################################################################

# Set the awesome font size for rlImGui
target_compile_definitions(rlImGui PRIVATE FONT_AWESOME_ICON_SIZE=26)

add_definitions(-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE})

# Setting ASSETS_PATH
if (CMAKE_BUILD_TYPE MATCHES "Debug")
    SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -O0 -DDEBUG")

    # Set the asset path macro to the absolute path on the dev machine
    target_compile_definitions(${PROJECT_NAME} PUBLIC ASSETS_PATH="${CMAKE_CURRENT_SOURCE_DIR}/assets/")
else()
    # Set the asset path macro in release mode to a relative path that assumes the assets folder is in the same directory as the game executable
    target_compile_definitions(${PROJECT_NAME} PUBLIC RESOURCES_PATH="./assets/")
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Release" AND NOT CMAKE_CXX_COMPILER_ID MATCHES "MSVC" AND NOT APPLE)
    message(STATUS "Release build: Applying size optimization flags.")
        target_compile_options(${PROJECT_NAME} PRIVATE 
        -ffunction-sections 
        -fdata-sections
    )
    target_link_options(${PROJECT_NAME} PRIVATE 
        "-Wl,--gc-sections" # Garbage collect unused sections
        "-s"                # Strip symbols from the executable
    )
endif()

# Set common compiler flags
# SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wall -Wswitch")
SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wall")