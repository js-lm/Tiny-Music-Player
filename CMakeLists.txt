cmake_minimum_required(VERSION 3.12)

project(tiny-music-player
    LANGUAGES CXX
    VERSION 0.2.0
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

set(SUPPORT_MODULE_RSHAPES  OFF CACHE BOOL "Disable rshapes module" FORCE)
set(SUPPORT_MODULE_RTEXT    OFF CACHE BOOL "Disable rtext module" FORCE)
set(SUPPORT_MODULE_RMODELS  OFF CACHE BOOL "Disable 3D models module" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES    OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG        5.5
)
FetchContent_MakeAvailable(raylib)

FetchContent_Declare(
    raygui
    GIT_REPOSITORY https://github.com/raysan5/raygui.git
    GIT_TAG        4.0
)
FetchContent_GetProperties(raygui)
if(NOT raygui_POPULATED)
    FetchContent_Populate(raygui)
endif()

FetchContent_Declare(
    fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG        12.1.0
)
FetchContent_MakeAvailable(fmt)

include(ExternalProject)

set(FFMPEG_PREFIX "${CMAKE_CURRENT_BINARY_DIR}/ffmpeg")
set(FFMPEG_INSTALL_DIR "${FFMPEG_PREFIX}/install")

ExternalProject_Add(ffmpeg
    URL "https://ffmpeg.org/releases/ffmpeg-7.0.1.tar.xz"
    PREFIX ${FFMPEG_PREFIX}
    CONFIGURE_COMMAND <SOURCE_DIR>/configure
        --prefix=${FFMPEG_INSTALL_DIR}
        --disable-shared
        --enable-static
        --disable-programs
        --disable-doc
        --disable-avdevice
        --disable-swscale
        --disable-postproc
        --disable-avfilter
        --disable-network
        --disable-everything
        --enable-decoder=mp1,mp2,mp3,mp3float,aac,flac,vorbis,alac,opus,ape,wavpack,dsd_lsbf,dsd_msbf,dsd_lsbf_planar,dsd_msbf_planar,tta,tak,shorten,ac3,eac3,dca,truehd,mlp,amrnb,amrwb,wmav1,wmav2,wmalossless,ra_144,ra_288,cook,gsm,g722,sbc,aptx,aptx_hd,speex,pcm_s16le,pcm_f32le,pcm_s16be,pcm_s24be,pcm_s32be,pcm_f64le,pcm_u8,pcm_alaw,pcm_mulaw,adpcm_g722,adpcm_g726,adpcm_ima_wav,adpcm_ms,adpcm_yamaha,adpcm_adx
        --enable-demuxer=mp3,mov,flac,ogg,wav,aac,opus,ape,wv,matroska,dsf,tta,tak,shorten,ac3,eac3,dts,dtshd,truehd,amr,asf,rm,flv,avi,nut,mpegts,caf,au,voc,w64,spx,webm,aiff,ircam,mlp,sbc,g722,gsm,adx,aptx
        --enable-parser=mpegaudio,aac,aac_latm,flac,vorbis,opus,ac3,dca,cook
        --enable-protocol=file
        --disable-bzlib
        --disable-vaapi
        --disable-vdpau
        --disable-libdrm
        --disable-vulkan
        --disable-x86asm
    BUILD_COMMAND make -j4
    INSTALL_COMMAND make install
)

include_directories(${FFMPEG_INSTALL_DIR}/include)
link_directories(${FFMPEG_INSTALL_DIR}/lib)

file(GLOB_RECURSE PROJECT_SOURCES
    CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/sources/*.cpp"
)

add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})

target_compile_definitions(${PROJECT_NAME} PRIVATE "PROJECT_VERSION=\"${PROJECT_VERSION}\"")
target_compile_definitions(${PROJECT_NAME} PRIVATE $<$<CONFIG:Debug>:DEBUG>)

target_include_directories(${PROJECT_NAME} PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/sources"
    "${CMAKE_CURRENT_SOURCE_DIR}/resources"
    "${raygui_SOURCE_DIR}/src"
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    raylib
    fmt
    avformat
    avcodec
    swresample
    avutil
    m
    z
    pthread
    X11
)

add_dependencies(${PROJECT_NAME} ffmpeg)