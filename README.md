# Tiny Music Player

A simple, lightweight, and cross platform music player built with C++, [raylib](https://www.raylib.com/), and [ImGui](https://github.com/ocornut/imgui).

<img width="600" height="160" alt="Screenshot from 2025-07-28 22-46-34" src="https://github.com/user-attachments/assets/92d25dec-885b-4fed-ba03-7fa2a645b14b" />

## Features

  * Supports `.wav`, `.ogg`, `.mp3`, `.qoa`, `.flac`, `.xm`, and `.mod`.
  * Minimalist UI
  * Four different loop modes: no loop, loop single track, loop directory, and loop directory infinitely.
  * Middle click the file path to open the song's containing folder.
  * Drag and drop an audio file onto the window to play it.

<img width="2880" height="1920" alt="Screenshot From 2025-07-28 22-45-48" src="https://github.com/user-attachments/assets/5b656bf3-3dec-4337-8da7-d0b05b4ac660" />

## Build

Install system dependecy `libid3tag` and let CMake do its magic

  * Debian: `sudo apt install libid3tag0-dev`
  * Fedora: `sudo dnf install libid3tag-devel`
