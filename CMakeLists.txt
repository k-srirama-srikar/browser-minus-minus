cmake_minimum_required(VERSION 3.20)
project(Browser)

# Use C++23 for modern error handling and logging
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# Find Packages
find_package(CURL REQUIRED)
find_package(Lua 5.4 REQUIRED)
find_package(PkgConfig REQUIRED)

# Try to find SDL3 locally
find_package(SDL3 QUIET)
if (NOT SDL3_FOUND)
    message(STATUS "SDL3 not found locally, fetching from GitHub...")
    FetchContent_Declare(
        SDL3
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG release-3.2.0
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(SDL3)
endif()

# Try to find SDL3_ttf locally
pkg_check_modules(SDL3_ttf QUIET IMPORTED_TARGET sdl3-ttf)
if (NOT SDL3_ttf_FOUND)
    message(STATUS "SDL3_ttf not found directly, fetching from GitHub...")
    FetchContent_Declare(
        SDL3_ttf
        GIT_REPOSITORY https://github.com/libsdl-org/SDL_ttf.git
        GIT_TAG release-3.2.0
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(SDL3_ttf)
endif()

# Include external and local headers
include_directories(include external/nlohmann/include/single_include external/sol2/include ${LUA_INCLUDE_DIR} ${CURL_INCLUDE_DIR})
if(sdl3_SOURCE_DIR)
    include_directories(${sdl3_SOURCE_DIR}/include)
endif()
if(sdl3_ttf_SOURCE_DIR)
    include_directories(${sdl3_ttf_SOURCE_DIR}/include)
endif()

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

# Conditional linking for SDL3_ttf depending on how it was found
if (TARGET PkgConfig::SDL3_ttf)
    set(SDL3_TTF_LINK_TARGET PkgConfig::SDL3_ttf)
elseif(TARGET SDL3_ttf-shared)
    set(SDL3_TTF_LINK_TARGET SDL3_ttf-shared)
elseif(TARGET SDL3_ttf::SDL3_ttf)
    set(SDL3_TTF_LINK_TARGET SDL3_ttf::SDL3_ttf)
else()
    set(SDL3_TTF_LINK_TARGET SDL3_ttf)
endif()

# Linking
target_link_libraries(browser 
    SDL3::SDL3 
    ${SDL3_TTF_LINK_TARGET}
    ${LUA_LIBRARY} 
    ${CURL_LIBRARY_RELEASE}
    pthread
)

# Optionally link SDL3_image if available
if(SDL3_image_FOUND)
    target_link_libraries(browser PkgConfig::SDL3_image)
    target_compile_definitions(browser PRIVATE HAS_SDL3_IMAGE=1)
endif()
