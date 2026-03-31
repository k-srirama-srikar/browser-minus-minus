/*
    Note this is just an inital template of sorts... these will be prone to many changes... 
*/


#include<SDL3/SDL.h>

bool initRenderer(SDL_Window** window, SDL_Renderer** renderer) {
    if (!SDL_Init(SDL_INIT_VIDEO)) return false;
    
    if (!SDL_CreateWindowAndRenderer("Browser", 1280, 720, 0, window, renderer)) {
        return false;
    }
    return true;
}