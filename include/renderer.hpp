#pragma once

#include <SDL3/SDL.h>
#include "dom.hpp"

bool initRenderer(SDL_Window** window, SDL_Renderer** renderer);
void renderDom(SDL_Renderer* renderer, const Node* root);
void shutdownRenderer();
