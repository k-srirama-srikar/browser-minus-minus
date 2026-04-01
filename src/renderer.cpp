#include "renderer.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>

static SDL_Window* g_window = nullptr;
static SDL_Renderer* g_renderer = nullptr;

bool initRenderer(SDL_Window** window, SDL_Renderer** renderer) {
    if (getenv("DISPLAY") == nullptr && getenv("WAYLAND_DISPLAY") == nullptr) {
        std::cerr << "No display detected. Switching to dummy video driver for headless mode." << std::endl;
        SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "dummy");
    }

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
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

    *window = g_window;
    *renderer = g_renderer;
    return true;
}

void shutdownRenderer() {
    if (g_renderer) {
        SDL_DestroyRenderer(g_renderer);
        g_renderer = nullptr;
    }
    if (g_window) {
        SDL_DestroyWindow(g_window);
        g_window = nullptr;
    }
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
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    const int dotWidth = 3;
    const int dotHeight = 3;
    const int spacing = 4;
    int cursor = x;
    for (unsigned char ch : text) {
        uint8_t pattern = ch;
        for (int bit = 0; bit < 8; ++bit) {
            if ((pattern >> bit) & 1u) {
                        SDL_FRect dot;
                dot.x = static_cast<float>(cursor + (bit % 4) * dotWidth);
                dot.y = static_cast<float>(y + (bit / 4) * dotHeight);
                dot.w = static_cast<float>(dotWidth);
                dot.h = static_cast<float>(dotHeight);
                SDL_RenderFillRect(renderer, &dot);
            }
        }
        cursor += dotWidth * 4 + spacing;
    }
}

static void renderNode(SDL_Renderer* renderer, const Node* node) {
    if (node->type == NodeType::FlexV || node->type == NodeType::FlexH) {
        drawRectangle(renderer, node, {36, 40, 52, 255}, true);
        drawRectangle(renderer, node, {78, 90, 110, 255}, false);
    } else if (node->type == NodeType::Image) {
        drawRectangle(renderer, node, {84, 100, 140, 255}, true);
        drawRectangle(renderer, node, {170, 190, 210, 255}, false);
        drawText(renderer, "IMG", static_cast<int>(node->x + 8.0f), static_cast<int>(node->y + 8.0f), {235, 235, 235, 255});
    } else if (node->type == NodeType::Text) {
        drawRectangle(renderer, node, {24, 30, 40, 255}, true);
        drawText(renderer, node->content, static_cast<int>(node->x + 6.0f), static_cast<int>(node->y + 6.0f), {230, 230, 230, 255});
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
