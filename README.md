# `Browser--`

## Overview

`Browser--` is a lightweight browser project written in modern C++ with Lua scripting support.

## Prerequisites

Before building this repository, install the following dependencies:

- A C++ compiler with C++23 support (for example `g++` or `clang++`)
- CMake version 3.20 or newer
- SDL3 development libraries
- SDL3_ttf development libraries
- libcurl development libraries
- Lua 5.4 development libraries
- pkg-config

### Debian / Ubuntu example

```bash
sudo apt update
sudo apt install -y build-essential cmake pkg-config libcurl4-openssl-dev libsdl3-dev libsdl3-ttf-dev liblua5.4-dev lua5.4
```

### Fedora / RHEL example

```bash
sudo dnf install -y gcc-c++ cmake pkgconf-pkg-config libcurl-devel SDL3-devel SDL3_ttf-devel lua-devel
```

> Note: package names may vary by distribution. Ensure you have development headers for SDL3, SDL3_ttf, libcurl, and Lua 5.4.

## Build Instructions

From the repository root:

```bash
mkdir -p build
cd build
cmake ..
cmake --build .
```

If you prefer an explicit generator or build type, use:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

## Run Instructions

After a successful build, the executable is available at `build/browser`.

To run the browser:

```bash
./build/browser
```

## Executing `index.jsml`

The repository includes `assets/index.jsml` as the starting script for the browser.

To execute it, launch the built executable and provide the path to `index.jsml` if supported by the application. For example:

```bash
./build/browser assets/index.jsml
```

If the executable does not accept a script path argument directly, place `assets/index.jsml` in the expected runtime location or modify the startup configuration in `src/main.cpp`.

## Notes

- `CMakeLists.txt` requires `find_package(SDL3 REQUIRED)`, `find_package(CURL REQUIRED)`, and `find_package(Lua 5.4 REQUIRED)`.
- `pkg_check_modules(SDL3_ttf REQUIRED IMPORTED_TARGET sdl3-ttf)` is used to link SDL2 TTF support.
- Ensure your environment has both the library binaries and development headers installed.
