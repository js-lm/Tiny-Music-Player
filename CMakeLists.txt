cmake_minimum_required(VERSION 3.12)

project(tiny-music-player
    LANGUAGES CXX
    VERSION 0.1.0
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)



# fetch contents
set(SUPPORT_MODULE_RSHAPES  OFF CACHE BOOL "Disable rshapes module" FORCE)
set(SUPPORT_MODULE_RTEXT    OFF CACHE BOOL "Disable rtext module" FORCE)
set(SUPPORT_MODULE_RMODELS  OFF CACHE BOOL "Disable 3D models module" FORCE)
set(SUPPORT_FILEFORMAT_FLAC  ON CACHE BOOL "Enable FLAC support" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES    OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG        5.5
)
FetchContent_MakeAvailable(raylib)

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
)
target_include_directories(rlImGui PUBLIC
    ${rlimgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}
)
target_link_libraries(rlImGui PUBLIC raylib)
target_compile_definitions(rlImGui PRIVATE FONT_AWESOME_ICON_SIZE=26)



# define PROJECT_SOURCES
file(GLOB_RECURSE PROJECT_SOURCES
    CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/sources/*.cpp"
)


# configure the build
add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})

target_compile_definitions(${PROJECT_NAME} PRIVATE "PROJECT_VERSION=\"${PROJECT_VERSION}\"")

target_include_directories(${PROJECT_NAME} PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/sources"
    "${CMAKE_CURRENT_SOURCE_DIR}/resources"
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    raylib
    rlImGui
)