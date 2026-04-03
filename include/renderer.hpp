#pragma once

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <string>
#include "dom.hpp"

// Returns a cached TTF_Font for the given point size.
// Falls back to a default if the size is unavailable.
TTF_Font* getFont(int ptSize);

// Measure the pixel dimensions of rendered text at a given font size.
// Returns {0,0} if the font is unavailable.
SDL_Point measureText(const std::string& text, int ptSize);

bool initRenderer(SDL_Window** window, SDL_Renderer** renderer);
void renderUrlBox(SDL_Renderer* renderer, const std::string& url, bool focused, int width, int height, int cursorPosition);
void renderDom(SDL_Renderer* renderer, const Node* root);
void shutdownRenderer();
