#include "renderer.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
#include <fstream>
#include <SDL3_ttf/SDL_ttf.h>

static SDL_Window* g_window = nullptr;
static SDL_Renderer* g_renderer = nullptr;
static TTF_Font* g_font = nullptr;
static std::unordered_map<int, TTF_Font*> g_fontCache;
static std::string g_fontPath;

static bool fileExists(const std::string& path) {
    if (path.empty()) {
        return false;
    }
    std::ifstream file(path);
    return file.is_open();
}

static std::string locateFontPath() {
    std::vector<std::string> candidates;
    const char* envFont = getenv("BROWSER_FONT");
    if (envFont && envFont[0]) {
        candidates.emplace_back(envFont);
    }
    const char* envFont2 = getenv("FONT");
    if (envFont2 && envFont2[0]) {
        candidates.emplace_back(envFont2);
    }

    // Local assets first (support various working directory cases)
    candidates.emplace_back("src/assets/DejaVuSans.ttf");
    candidates.emplace_back("src/assets/LiberationSans-Regular.ttf");
    candidates.emplace_back("assets/DejaVuSans.ttf");
    candidates.emplace_back("assets/LiberationSans-Regular.ttf");
    candidates.emplace_back("../src/assets/DejaVuSans.ttf");
    candidates.emplace_back("../src/assets/LiberationSans-Regular.ttf");

    // System fonts as fallback
    candidates.emplace_back("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf");
    candidates.emplace_back("/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf");
    candidates.emplace_back("/usr/share/fonts/truetype/freefont/FreeSans.ttf");
    candidates.emplace_back("/Library/Fonts/Arial.ttf");
    candidates.emplace_back("/System/Library/Fonts/SFNS.ttf");
    candidates.emplace_back("C:\\Windows\\Fonts\\arial.ttf");

    for (const std::string& candidate : candidates) {
        if (fileExists(candidate)) {
            std::cout << "Font candidate found: " << candidate << std::endl;
            return candidate;
        }
    }
    std::cerr << "No font candidate found in search path." << std::endl;
    return std::string();
}

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

    g_fontPath = locateFontPath();
    if (!g_fontPath.empty()) {
        g_font = TTF_OpenFont(g_fontPath.c_str(), 16);
        if (!g_font) {
            std::cerr << "Warning: Failed to load font at " << g_fontPath << ": " << SDL_GetError() << std::endl;
        } else {
            g_fontCache[16] = g_font;
        }
    } else {
        std::cerr << "Warning: No system font found; text rendering will be disabled unless BROWSER_FONT is set." << std::endl;
    }

    *window = g_window;
    *renderer = g_renderer;
    return true;
}

static TTF_Font* getFontForSize(int size) {
    if (size <= 0) {
        return g_font;
    }
    auto it = g_fontCache.find(size);
    if (it != g_fontCache.end()) {
        return it->second;
    }
    if (g_fontPath.empty()) {
        return g_font;
    }
    TTF_Font* font = TTF_OpenFont(g_fontPath.c_str(), size);
    if (font) {
        g_fontCache[size] = font;
        return font;
    }
    return g_font;
}

static SDL_Color parseHexColor(const std::string& text, SDL_Color fallback) {
    if (text.empty() || text[0] != '#') return fallback;
    unsigned int value = 0;
    try {
        value = std::stoul(text.substr(1), nullptr, 16);
    } catch (...) {
        return fallback;
    }
    if (text.size() == 7) {
        return { static_cast<Uint8>((value >> 16) & 0xFF), static_cast<Uint8>((value >> 8) & 0xFF), static_cast<Uint8>(value & 0xFF), 255 };
    }
    if (text.size() == 9) {
        return { static_cast<Uint8>((value >> 24) & 0xFF), static_cast<Uint8>((value >> 16) & 0xFF), static_cast<Uint8>((value >> 8) & 0xFF), static_cast<Uint8>(value & 0xFF) };
    }
    return fallback;
}

static int parseStyleInt(const Node* node, const std::string& key, int fallback) {
    if (node->properties.contains(key)) {
        if (node->properties[key].is_number_integer()) {
            return node->properties[key].get<int>();
        }
        if (node->properties[key].is_number()) {
            return static_cast<int>(node->properties[key].get<float>());
        }
    }
    if (node->properties.contains("text") && node->properties["text"].is_object()) {
        auto& textObj = node->properties["text"];
        if (textObj.contains(key)) {
            if (textObj[key].is_number_integer()) {
                return textObj[key].get<int>();
            }
            if (textObj[key].is_number()) {
                return static_cast<int>(textObj[key].get<float>());
            }
        }
    }
    return fallback;
}

static SDL_Color parseStyleColor(const Node* node, const std::string& key, SDL_Color fallback) {
    if (node->properties.contains(key) && node->properties[key].is_string()) {
        return parseHexColor(node->properties[key].get<std::string>(), fallback);
    }
    if (node->properties.contains("text") && node->properties["text"].is_object()) {
        auto& textObj = node->properties["text"];
        if (textObj.contains(key) && textObj[key].is_string()) {
            return parseHexColor(textObj[key].get<std::string>(), fallback);
        }
    }
    return fallback;
}

static void cleanupFontCache() {
    for (auto& [size, font] : g_fontCache) {
        if (font) {
            TTF_CloseFont(font);
        }
    }
    g_fontCache.clear();
}

void shutdownRenderer() {
    cleanupFontCache();
    g_font = nullptr;
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

static void drawText(SDL_Renderer* renderer, const std::string& text, int x, int y, SDL_Color color, int fontSize = 16) {
    if (text.empty()) return;
    TTF_Font* font = getFontForSize(fontSize);
    if (!font) {
        return;
    }

    SDL_Surface* surface = TTF_RenderText_Blended(font, text.c_str(), text.length(), color);
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
        SDL_Color fillColor = parseStyleColor(node, "bgcolor", {36, 40, 52, 255});
        drawRectangle(renderer, node, fillColor, true);
        drawRectangle(renderer, node, {78, 90, 110, 255}, false);
    } else if (node->type == NodeType::Image) {
        drawRectangle(renderer, node, {84, 100, 140, 255}, true);
        drawRectangle(renderer, node, {170, 190, 210, 255}, false);
        drawText(renderer, "IMG", static_cast<int>(node->x + 8.0f), static_cast<int>(node->y + 8.0f), {235, 235, 235, 255});
    } else if (node->type == NodeType::Text) {
        SDL_Color bgColor = parseStyleColor(node, "bgcolor", {24, 30, 40, 255});
        SDL_Color textColor = parseStyleColor(node, "color", {230, 230, 230, 255});
        int fontSize = parseStyleInt(node, "fontsize", 16);
        drawRectangle(renderer, node, bgColor, true);
        drawText(renderer, getNodeText(node), static_cast<int>(node->x + 6.0f), static_cast<int>(node->y + 6.0f), textColor, fontSize);
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
