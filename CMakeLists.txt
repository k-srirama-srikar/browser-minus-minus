cmake_minimum_required(VERSION 3.20)
project(Browser)

# Use C++23 for modern error handling and logging
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Packages
find_package(SDL3 REQUIRED)
find_package(CURL REQUIRED)
find_package(Lua 5.4 REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL3_ttf REQUIRED IMPORTED_TARGET sdl3-ttf)

# Include external and local headers
include_directories(include external/nlohmann/include/single_include external/sol2/include ${LUA_INCLUDE_DIR} ${CURL_INCLUDE_DIR})

# Source files
set(SOURCES 
    src/main.cpp 
    src/dom.cpp 
    src/layout.cpp 
    src/renderer.cpp 
    src/scripting.cpp
    src/network.cpp
)

add_executable(browser ${SOURCES})

# Linking
target_link_libraries(browser 
    SDL3::SDL3 
    PkgConfig::SDL3_ttf
    ${LUA_LIBRARY} 
    ${CURL_LIBRARY_RELEASE}
    pthread
)
