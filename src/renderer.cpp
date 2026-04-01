#include "renderer.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <SDL3_ttf/SDL_ttf.h>

static SDL_Window* g_window = nullptr;
static SDL_Renderer* g_renderer = nullptr;
static TTF_Font* g_font = nullptr;

bool initRenderer(SDL_Window** window, SDL_Renderer** renderer) {
    if (getenv("DISPLAY") == nullptr && getenv("WAYLAND_DISPLAY") == nullptr) {
        std::cerr << "No display detected. Switching to dummy video driver for headless mode." << std::endl;
        SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "dummy");
    }

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return false;
    }
    
    if (!TTF_Init()) {
        std::cerr << "TTF_Init Error: " << SDL_GetError() << std::endl;
        return false;
    }

    g_window = SDL_CreateWindow("Browser++ Toy Browser", 1280, 720, 0);
    if (!g_window) {
        std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return false;
    }

    g_renderer = SDL_CreateRenderer(g_window, nullptr);
    if (!g_renderer) {
        std::cerr << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(g_window);
        SDL_Quit();
        return false;
    }

    g_font = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 16.0f);
    if (!g_font) {
        std::cerr << "Warning: Failed to load DejaVuSans.ttf: " << SDL_GetError() << std::endl;
    }

    *window = g_window;
    *renderer = g_renderer;
    return true;
}

void shutdownRenderer() {
    if (g_font) {
        TTF_CloseFont(g_font);
        g_font = nullptr;
    }
    if (g_renderer) {
        SDL_DestroyRenderer(g_renderer);
        g_renderer = nullptr;
    }
    if (g_window) {
        SDL_DestroyWindow(g_window);
        g_window = nullptr;
    }
    TTF_Quit();
    SDL_Quit();
}

static void drawRectangle(SDL_Renderer* renderer, const Node* node, SDL_Color color, bool fill) {
    SDL_FRect rect;
    rect.x = node->x;
    rect.y = node->y;
    rect.w = node->width;
    rect.h = node->height;
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    if (fill) {
        SDL_RenderFillRect(renderer, &rect);
    } else {
        SDL_RenderRect(renderer, &rect);
    }
}

static void drawText(SDL_Renderer* renderer, const std::string& text, int x, int y, SDL_Color color) {
    if (text.empty()) return;
    
    if (!g_font) {
        // Fallback or silently drop text if no font loaded
        return;
    }

    SDL_Surface* surface = TTF_RenderText_Blended(g_font, text.c_str(), text.length(), color);
    if (surface) {
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        if (texture) {
            SDL_FRect dest = { static_cast<float>(x), static_cast<float>(y), static_cast<float>(surface->w), static_cast<float>(surface->h) };
            SDL_RenderTexture(renderer, texture, nullptr, &dest);
            SDL_DestroyTexture(texture);
        }
        SDL_DestroySurface(surface);
    }
}

static std::string getNodeText(const Node* node) {
    if (node->properties.contains("text") && node->properties["text"].is_object()) {
        if (node->properties["text"].contains("content") && node->properties["text"]["content"].is_string()) {
            return node->properties["text"]["content"].get<std::string>();
        }
    }
    return "";
}

static void renderNode(SDL_Renderer* renderer, const Node* node) {
    if (node->type == NodeType::Root) {
        SDL_SetRenderDrawColor(renderer, 24, 26, 32, 255);
        SDL_RenderClear(renderer);
    } else if (node->type == NodeType::FlexV || node->type == NodeType::FlexH) {
        drawRectangle(renderer, node, {36, 40, 52, 255}, true);
        drawRectangle(renderer, node, {78, 90, 110, 255}, false);
    } else if (node->type == NodeType::Image) {
        drawRectangle(renderer, node, {84, 100, 140, 255}, true);
        drawRectangle(renderer, node, {170, 190, 210, 255}, false);
        drawText(renderer, "IMG", static_cast<int>(node->x + 8.0f), static_cast<int>(node->y + 8.0f), {235, 235, 235, 255});
    } else if (node->type == NodeType::Text) {
        drawRectangle(renderer, node, {24, 30, 40, 255}, true);
        drawText(renderer, getNodeText(node), static_cast<int>(node->x + 6.0f), static_cast<int>(node->y + 6.0f), {230, 230, 230, 255});
    }

    for (const Node* child : node->children) {
        renderNode(renderer, child);
    }
}

void renderDom(SDL_Renderer* renderer, const Node* root) {
    if (!renderer || !root) {
        return;
    }
    renderNode(renderer, root);
}
