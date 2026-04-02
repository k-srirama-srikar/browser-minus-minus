# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/home/shravani/browser/browser-minus-minus/build/_deps/sdl3-src"
  "/home/shravani/browser/browser-minus-minus/build/_deps/sdl3-build"
  "/home/shravani/browser/browser-minus-minus/build/_deps/sdl3-subbuild/sdl3-populate-prefix"
  "/home/shravani/browser/browser-minus-minus/build/_deps/sdl3-subbuild/sdl3-populate-prefix/tmp"
  "/home/shravani/browser/browser-minus-minus/build/_deps/sdl3-subbuild/sdl3-populate-prefix/src/sdl3-populate-stamp"
  "/home/shravani/browser/browser-minus-minus/build/_deps/sdl3-subbuild/sdl3-populate-prefix/src"
  "/home/shravani/browser/browser-minus-minus/build/_deps/sdl3-subbuild/sdl3-populate-prefix/src/sdl3-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/shravani/browser/browser-minus-minus/build/_deps/sdl3-subbuild/sdl3-populate-prefix/src/sdl3-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/shravani/browser/browser-minus-minus/build/_deps/sdl3-subbuild/sdl3-populate-prefix/src/sdl3-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
