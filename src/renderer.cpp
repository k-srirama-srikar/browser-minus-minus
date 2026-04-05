#include "renderer.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
#include <fstream>
#include "tab.hpp"
#include <SDL3_ttf/SDL_ttf.h>

#ifdef HAS_SDL3_IMAGE
#include <SDL3_image/SDL_image.h>
#endif

static SDL_Window* g_window = nullptr;
static SDL_Renderer* g_renderer = nullptr;
static TTF_Font* g_font = nullptr;
static std::unordered_map<int, TTF_Font*> g_fontCache;
static std::unordered_map<std::string, SDL_Texture*> g_imageCache;
static std::string g_fontPath;
static std::vector<std::pair<int, SDL_FRect>> g_tabRects;
static std::vector<std::pair<int, SDL_FRect>> g_closeRects;

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
            // std::cout << "Font candidate found: " << candidate << std::endl;
            return candidate;
        }
    }
    // std::cerr << "No font candidate found in search path." << std::endl;
    return std::string();
}

static SDL_Texture* loadImageTexture(const std::string& path) {
    if (path.empty()) return nullptr;

    // Check cache first
    auto it = g_imageCache.find(path);
    if (it != g_imageCache.end()) {
        return it->second;
    }

    SDL_Texture* texture = nullptr;
    
#ifdef HAS_SDL3_IMAGE
    // Use SDL_image for loading various formats
    SDL_Surface* surface = IMG_Load(path.c_str());
    if (surface) {
        texture = SDL_CreateTextureFromSurface(g_renderer, surface);
        SDL_DestroySurface(surface);
        if (texture) {
            g_imageCache[path] = texture;
            // std::cout << "Image loaded: " << path << std::endl;
        }
    } else {
        // std::cerr << "Failed to load image " << path << ": " << SDL_GetError() << std::endl;
    }
#else
    std::cerr << "SDL3_image not available; cannot load image: " << path << std::endl;
#endif
    
    return texture;
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

static TTF_Font* getFontForSize(int size);

TTF_Font* getFont(int ptSize) {
    return getFontForSize(ptSize);
}

SDL_Point measureText(const std::string& text, int ptSize) {
    if (text.empty()) {
        return {0, 0};
    }
    TTF_Font* font = getFontForSize(ptSize);
    if (!font) {
        return {0, 0};
    }
    int w = 0, h = 0;
    if (TTF_GetStringSize(font, text.c_str(), text.length(), &w, &h)) {
        return {w, h};
    }
    return {0, 0};
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

static void cleanupImageCache() {
    for (auto& [path, texture] : g_imageCache) {
        if (texture) {
            SDL_DestroyTexture(texture);
        }
    }
    g_imageCache.clear();
}

void shutdownRenderer() {
    cleanupFontCache();
    cleanupImageCache();
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

static void drawRoundedRect(SDL_Renderer* renderer, float x, float y, float w, float h, float radius, SDL_Color color, bool fill) {
    if (radius <= 0.0f) {
        SDL_FRect rect = {x, y, w, h};
        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
        if (fill) SDL_RenderFillRect(renderer, &rect);
        else SDL_RenderRect(renderer, &rect);
        return;
    }

    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    
    if (fill) {
        // Center part
        SDL_FRect center = {x + radius, y, w - 2 * radius, h};
        SDL_RenderFillRect(renderer, &center);
        // Left part
        SDL_FRect left = {x, y + radius, radius, h - 2 * radius};
        SDL_RenderFillRect(renderer, &left);
        // Right part
        SDL_FRect right = {x + w - radius, y + radius, radius, h - 2 * radius};
        SDL_RenderFillRect(renderer, &right);
        
        // Corners: draw points for simplicity or use a circle algorithm
        // For a high-performance browser, we'd use a shader or pre-rendered textures.
        // Here we'll just fill the corners with small rects for a "pixelated" rounded look if radius is small,
        // or just accept slightly squared corners for now to keep it simple and fast.
        // Actually, let's just use 4 rectangles for the main body and 4 small ones for corners.
    } else {
        // Just draw lines for the border
        SDL_RenderLine(renderer, x + radius, y, x + w - radius, y);
        SDL_RenderLine(renderer, x + radius, y + h, x + w - radius, y + h);
        SDL_RenderLine(renderer, x, y + radius, x, y + h - radius);
        SDL_RenderLine(renderer, x + w, y + radius, x + w, y + h - radius);
    }
}

static void drawVerticalGradient(SDL_Renderer* renderer, float x, float y, float w, float h, SDL_Color top, SDL_Color bottom) {
    for (int i = 0; i < (int)h; ++i) {
        float t = (float)i / h;
        Uint8 r = (Uint8)(top.r * (1.0f - t) + bottom.r * t);
        Uint8 g = (Uint8)(top.g * (1.0f - t) + bottom.g * t);
        Uint8 b = (Uint8)(top.b * (1.0f - t) + bottom.b * t);
        Uint8 a = (Uint8)(top.a * (1.0f - t) + bottom.a * t);
        SDL_SetRenderDrawColor(renderer, r, g, b, a);
        SDL_RenderLine(renderer, x, y + i, x + w, y + i);
    }
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
        // Only draw the background in the viewport area.
        // Screen clearing is handled in main.cpp.
        SDL_FRect contentRect = {node->x, node->y, node->width, node->height};
        SDL_SetRenderDrawColor(renderer, 24, 26, 32, 255);
        SDL_RenderFillRect(renderer, &contentRect);
    } else if (node->type == NodeType::FlexV || node->type == NodeType::FlexH) {
        SDL_Color fillColor = parseStyleColor(node, "bgcolor", {36, 40, 52, 255});
        drawRectangle(renderer, node, fillColor, true);
        drawRectangle(renderer, node, {78, 90, 110, 255}, false);
    } else if (node->type == NodeType::Image) {
        std::string altText = "[Image]";
        std::string imageUrl;
        SDL_Texture* imageTexture = nullptr;
        
        if (node->properties.contains("image") && node->properties["image"].is_object()) {
            if (node->properties["image"].contains("alttext") && node->properties["image"]["alttext"].is_string()) {
                altText = node->properties["image"]["alttext"].get<std::string>();
            }
            if (node->properties["image"].contains("url") && node->properties["image"]["url"].is_string()) {
                imageUrl = node->properties["image"]["url"].get<std::string>();
                imageTexture = loadImageTexture(imageUrl);
            }
        }
        
        // Draw background
        SDL_Color bgColor = parseStyleColor(node, "bgcolor", {84, 100, 140, 255});
        drawRectangle(renderer, node, bgColor, true);
        drawRectangle(renderer, node, {170, 190, 210, 255}, false);
        
        // Try to render actual image if loaded
        if (imageTexture) {
            SDL_FRect dest = {node->x, node->y, node->width, node->height};
            SDL_RenderTexture(renderer, imageTexture, nullptr, &dest);
        } else {
            // Fallback: render alttext with URL info
            drawText(renderer, altText, static_cast<int>(node->x + 8.0f), static_cast<int>(node->y + 8.0f), {235, 235, 235, 255}, 12);
            if (!imageUrl.empty()) {
                drawText(renderer, "URL: " + imageUrl, static_cast<int>(node->x + 8.0f), static_cast<int>(node->y + 28.0f), {180, 180, 180, 255}, 10);
            }
        }
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

void renderUrlBox(SDL_Renderer* renderer, const std::string& url, bool focused, int width, int height, int cursorPosition, int yOffset) {
    if (!renderer) return;

    // Background for Chrome URL area (below tabs)
    SDL_FRect chromeRect = {0.0f, static_cast<float>(yOffset), static_cast<float>(width), static_cast<float>(height)};
    SDL_SetRenderDrawColor(renderer, 53, 54, 58, 255); // Standard Chrome Dark bar color
    SDL_RenderFillRect(renderer, &chromeRect);

    // Separator line (very subtle)
    SDL_SetRenderDrawColor(renderer, 60, 64, 67, 255);
    SDL_RenderLine(renderer, 0, (float)yOffset, (float)width, (float)yOffset);

    // URL Box border/background (Pill shape)
    float boxMarginX = 80.0f;
    float boxMarginY = 6.0f;
    float boxX = boxMarginX;
    float boxY = (float)yOffset + boxMarginY;
    float boxW = (float)width - 2 * boxMarginX;
    float boxH = (float)height - 2 * boxMarginY;
    
    // Draw the pill background
    SDL_Color boxColor = {41, 42, 45, 255}; // Darker inset
    drawRoundedRect(renderer, boxX, boxY, boxW, boxH, boxH / 2.0f, boxColor, true);
    
    // Draw focus/hover border
    if (focused) {
        SDL_SetRenderDrawColor(renderer, 138, 180, 248, 255); // Chrome blue
        drawRoundedRect(renderer, boxX - 1, boxY - 1, boxW + 2, boxH + 2, (boxH + 2) / 2.0f, {138, 180, 248, 255}, false);
    } else {
        SDL_SetRenderDrawColor(renderer, 95, 99, 104, 255); // Mid gray
        drawRoundedRect(renderer, boxX, boxY, boxW, boxH, boxH / 2.0f, {95, 99, 104, 255}, false);
    }

    // Render URL text
    std::string displayUrl = url;
    float textX = boxX + 20.0f;
    float textY = boxY + (boxH - 18.0f) / 2.0f;

    if (displayUrl.empty() && !focused) {
        displayUrl = "Search or enter address";
        drawText(renderer, displayUrl, (int)textX, (int)textY, {154, 160, 166, 255}, 14);
    } else {
        drawText(renderer, displayUrl, (int)textX, (int)textY, {232, 234, 237, 255}, 14);

        // Render cursor if focused
        if (focused && (SDL_GetTicks() / 500) % 2 == 0) {
            int cursorPos = std::clamp(cursorPosition, 0, static_cast<int>(displayUrl.size()));
            std::string prefix = displayUrl.substr(0, cursorPos);
            SDL_Point prefixSize = measureText(prefix, 14);
            SDL_FRect cursorRect = {textX + static_cast<float>(prefixSize.x), textY + 2, 2.0f, boxH - 10};
            SDL_SetRenderDrawColor(renderer, 138, 180, 248, 255);
            SDL_RenderFillRect(renderer, &cursorRect);
        }
    }
}

void renderTabBar(SDL_Renderer* renderer, TabManager& tabManager, 
                          int activeTabId, int windowWidth) {
    const int TAB_HEIGHT = 42; // Increased for better proportions
    const int TAB_WIDTH = 200;
    const int TAB_SPACING = 2; 
    const int CLOSE_BUTTON_SIZE = 16;
    
    g_tabRects.clear();
    g_closeRects.clear();
    
    // Background for tab bar area (Chrome top section)
    SDL_FRect bgRect = {0, 0, static_cast<float>(windowWidth), static_cast<float>(TAB_HEIGHT)};
    SDL_SetRenderDrawColor(renderer, 32, 33, 36, 255); // Standard Chrome Dark bg
    SDL_RenderFillRect(renderer, &bgRect);
    
    int x = 8;
    auto tabIds = tabManager.getAllTabIds();
    
    for (int tabId : tabIds) {
        Tab* tab = tabManager.getTab(tabId);
        if (!tab) continue;
        
        bool isActive = (tabId == activeTabId);
        
        float tabX = static_cast<float>(x);
        float tabY = 8.0f; // Padding from top
        float tabW = static_cast<float>(TAB_WIDTH);
        float tabH = static_cast<float>(TAB_HEIGHT - 8);
        
        SDL_FRect tabRect = {tabX, tabY, tabW, tabH};
        
        // Tab shape: Rounded top corners
        SDL_Color tabColor = isActive ? SDL_Color{53, 54, 58, 255} : SDL_Color{41, 42, 45, 255};
        
        // Draw rounded tab
        drawRoundedRect(renderer, tabX, tabY, tabW, tabH, 8.0f, tabColor, true);
        
        if (isActive) {
            // Subtle top highlight for active tab
            SDL_FRect topHighlight = {tabX + 8, tabY, tabW - 16, 2};
            SDL_SetRenderDrawColor(renderer, 138, 180, 248, 255); // Chrome blue
            SDL_RenderFillRect(renderer, &topHighlight);
        } else {
            // Draw a subtle separator if not active and not first
            if (x > 8) {
                SDL_SetRenderDrawColor(renderer, 60, 64, 67, 255);
                SDL_RenderLine(renderer, tabX - 1, tabY + 8, tabX - 1, tabY + tabH - 8);
            }
        }
        
        // Label (Tab Title/URL)
        std::string label = tab->getUrl();
        if (label.empty()) label = "New Tab";
        else {
            size_t lastSlash = label.find_last_of("/\\");
            if (lastSlash != std::string::npos && lastSlash + 1 < label.length()) {
                label = label.substr(lastSlash + 1);
            }
        }
        if (label.length() > 20) label = label.substr(0, 17) + "...";
        
        SDL_Color textColor = isActive ? SDL_Color{232, 234, 237, 255} : SDL_Color{154, 160, 166, 255};
        drawText(renderer, label, (int)tabX + 12, (int)tabY + 8, textColor, 13);
        
        // Close button (x) - only show on active tab or hover (simplified to always for now)
        float cx = tabX + tabW - 24;
        float cy = tabY + (tabH - CLOSE_BUTTON_SIZE) / 2;
        SDL_FRect closeRect = {cx, cy, (float)CLOSE_BUTTON_SIZE, (float)CLOSE_BUTTON_SIZE};
        g_closeRects.push_back({tabId, closeRect});
        
        // Draw 'x' with a circle background on active
        if (isActive) {
            drawRoundedRect(renderer, cx - 2, cy - 2, CLOSE_BUTTON_SIZE + 4, CLOSE_BUTTON_SIZE + 4, 10.0f, {72, 75, 78, 255}, true);
        }
        drawText(renderer, "x", (int)cx + 4, (int)cy - 1, {180, 180, 180, 255}, 12);
        
        g_tabRects.push_back({tabId, tabRect});
        x += TAB_WIDTH + TAB_SPACING;
    }
    
    // "+" button (New Tab)
    float plusX = (float)x + 8;
    float plusY = 12.0f;
    float plusSize = 24.0f;
    SDL_FRect plusRect = {plusX, plusY, plusSize, plusSize};
    
    // Draw stylish circular plus button
    drawRoundedRect(renderer, plusX, plusY, plusSize, plusSize, 12.0f, {60, 64, 67, 255}, true);
    drawText(renderer, "+", (int)plusX + 7, (int)plusY + 1, {218, 220, 224, 255}, 18);
    
    g_tabRects.push_back({-99, plusRect}); 
}

int getTabAtPosition(float x, float y) {
    for (const auto& [tabId, rect] : g_tabRects) {
        if (x >= rect.x && x <= rect.x + rect.w &&
            y >= rect.y && y <= rect.y + rect.h) {
            return tabId;
        }
    }
    return -1;
}

int getCloseButtonTabAtPosition(float x, float y) {
    for (const auto& [tabId, rect] : g_closeRects) {
        if (x >= rect.x && x <= rect.x + rect.w &&
            y >= rect.y && y <= rect.y + rect.h) {
            return tabId;
        }
    }
    return -1;
}

void renderDom(SDL_Renderer* renderer, const Node* root) {
    if (!renderer || !root) {
        return;
    }
    renderNode(renderer, root);
}
