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
#include "url_utils.hpp"
#include "network.hpp"
#include "utils.hpp"

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
static float g_tabScrollOffset = 0.0f;
static float g_urlScrollOffset = 0.0f;
static float g_maxTabScroll = 0.0f;

float getTabScrollOffset() { return g_tabScrollOffset; }
void setTabScrollOffset(float offset) { 
    g_tabScrollOffset = std::max(0.0f, std::min(offset, g_maxTabScroll)); 
}

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
    if (UrlUtils::isNetworkUrl(path)) {
        std::string data;
        if (fetchUrl(path, data)) {
            SDL_IOStream* io = SDL_IOFromMem((void*)data.data(), data.size());
            if (io) {
                SDL_Surface* surface = IMG_Load_IO(io, true);
                if (surface) {
                    texture = SDL_CreateTextureFromSurface(g_renderer, surface);
                    SDL_DestroySurface(surface);
                    if (texture) {
                        g_imageCache[path] = texture;
                    }
                }
            }
        }
    } else {
        SDL_Surface* surface = IMG_Load(path.c_str());
        if (surface) {
            texture = SDL_CreateTextureFromSurface(g_renderer, surface);
            SDL_DestroySurface(surface);
            if (texture) {
                g_imageCache[path] = texture;
            }
        }
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
    if (text.size() == 8) { // Handle 7 hex digits (rare but in docs)
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

static void drawSquaredBox(SDL_Renderer* renderer, float x, float y, float w, float h, SDL_Color bgColor, SDL_Color borderColor, bool hasBorder) {
    SDL_FRect rect = {x, y, w, h};
    
    // Fill background
    SDL_SetRenderDrawColor(renderer, bgColor.r, bgColor.g, bgColor.b, bgColor.a);
    SDL_RenderFillRect(renderer, &rect);
    
    // Draw border in a single pass
    if (hasBorder) {
        SDL_SetRenderDrawColor(renderer, borderColor.r, borderColor.g, borderColor.b, borderColor.a);
        SDL_RenderRect(renderer, &rect);
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
    if (!node) return;
    SDL_FRect rect;
    rect.x = node->x;
    rect.y = node->y;
    rect.w = node->width;
    rect.h = node->height;
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    if (fill) {
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_RenderFillRect(renderer, &rect);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    } else {
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_RenderRect(renderer, &rect);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    }
}

static void drawText(SDL_Renderer* renderer, const std::string& text, int x, int y, int maxWidth, SDL_Color color, int fontSize = 16) {
    if (text.empty() || maxWidth <= 0) return;
    TTF_Font* font = getFontForSize(fontSize);
    if (!font) return;

    SDL_Surface* surface = TTF_RenderText_Blended_Wrapped(font, text.c_str(), text.length(), color, maxWidth);
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

static void drawTextSingleLine(SDL_Renderer* renderer, const std::string& text, int x, int y, SDL_Color color, int fontSize = 16) {
    if (text.empty()) return;
    TTF_Font* font = getFontForSize(fontSize);
    if (!font) return;

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

static void renderNode(SDL_Renderer* renderer, const Node* node, const std::string& baseUrl) {
    if (node->type == NodeType::Root) {
        SDL_FRect contentRect = {node->x, node->y, node->width, node->height};
        SDL_SetRenderDrawColor(renderer, 24, 26, 32, 255);
        SDL_RenderFillRect(renderer, &contentRect);
    } else {
        // Draw background for all themed nodes
        SDL_Color bgColor = parseStyleColor(node, "bgcolor", {0, 0, 0, 0});
        if (bgColor.a > 0) {
            drawRectangle(renderer, node, bgColor, true);
        }
        
        // Draw border if requested or if it's a container
        SDL_Color borderColor = parseStyleColor(node, "bordercolor", {0, 0, 0, 0});
        if (borderColor.a > 0) {
            drawRectangle(renderer, node, borderColor, false);
        } else if (node->type == NodeType::FlexV || node->type == NodeType::FlexH) {
            // Default subtle border for sections
            drawRectangle(renderer, node, {78, 90, 110, 255}, false);
        }

        // Render Image if applicable
        if (node->type == NodeType::Image || node->properties.contains("image")) {
             std::string altText = "[Image]";
             std::string imageUrl;
             SDL_Texture* imageTexture = nullptr;
             
             if (node->properties.contains("image") && node->properties["image"].is_object()) {
                 if (node->properties["image"].contains("alttext") && node->properties["image"]["alttext"].is_string()) {
                     altText = node->properties["image"]["alttext"].get<std::string>();
                 }
                 if (node->properties["image"].contains("url") && node->properties["image"]["url"].is_string()) {
                     imageUrl = node->properties["image"]["url"].get<std::string>();
                     std::string resolvedImageUrl = UrlUtils::resolvePath(baseUrl, imageUrl);
                     imageTexture = loadImageTexture(resolvedImageUrl);
                 }
             }
             
             if (imageTexture) {
                 SDL_FRect dest = {node->x, node->y, node->width, node->height};
                 SDL_RenderTexture(renderer, imageTexture, nullptr, &dest);
             } else if (!imageUrl.empty() || node->type == NodeType::Image) {
                 drawText(renderer, altText, static_cast<int>(node->x + 8.0f), static_cast<int>(node->y + 8.0f), static_cast<int>(node->width - 16.0f), {235, 235, 235, 255}, 12);
             }
        }

        // Render Text if applicable (works for all nodes now)
        std::string content = getNodeText(node);
        if (!content.empty()) {
            SDL_Color textColor = parseStyleColor(node, "color", {230, 230, 230, 255});
            int fontSize = parseStyleInt(node, "fontsize", 16);
            
            // Calculate vertical centering
            SDL_Point size = measureText(content, fontSize);
            float textY = node->y + (node->height - (float)size.y) / 2.0f;
            if (textY < node->y + 4.0f) textY = node->y + 4.0f; // Minimal top padding

            drawText(renderer, content, static_cast<int>(node->x + 12.0f), static_cast<int>(textY), static_cast<int>(node->width - 24.0f), textColor, fontSize);
        }
    }

    for (const Node* child : node->children) {
        renderNode(renderer, child, baseUrl);
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

    // URL Box background and border in a single pass (Squared edges)
    float boxMarginX = 80.0f;
    float boxMarginY = 7.0f;
    float boxX = boxMarginX;
    float boxY = (float)yOffset + boxMarginY;
    float boxW = (float)width - 2 * boxMarginX;
    float boxH = (float)height - 2 * boxMarginY;
    
    SDL_Color boxBg = {30, 31, 34, 255};
    SDL_Color boxBorder = focused ? SDL_Color{100, 150, 255, 255} : SDL_Color{60, 64, 67, 255};
    drawSquaredBox(renderer, boxX, boxY, boxW, boxH, boxBg, boxBorder, true);

    // Set clipping viewport for text scrolling
    SDL_Rect prevViewport;
    SDL_GetRenderViewport(renderer, &prevViewport);
    SDL_Rect urlViewport = {(int)boxX + 5, (int)boxY, (int)boxW - 10, (int)boxH};
    SDL_SetRenderViewport(renderer, &urlViewport);

    // Render URL text with scrolling
    std::string displayUrl = url;
    float textX = 10.0f; // Relative to viewport
    float textY = (boxH - 18.0f) / 2.0f;

    SDL_Point totalSize = measureText(displayUrl, 14);
    int cursorX = measureText(displayUrl.substr(0, cursorPosition), 14).x;
    float cursorVisualX = (float)cursorX - g_urlScrollOffset;
    float margin = 30.0f;

    if (cursorVisualX < margin) {
        g_urlScrollOffset = std::max(0.0f, (float)cursorX - margin);
    } else if (cursorVisualX > boxW - margin) {
        g_urlScrollOffset = (float)cursorX - (boxW - margin);
    }
    
    // Ensure scroll offset is sane
    float maxUrlScroll = std::max(0.0f, (float)totalSize.x - (boxW - 20.0f));
    if (g_urlScrollOffset > maxUrlScroll) g_urlScrollOffset = maxUrlScroll;

    if (displayUrl.empty() && !focused) {
        displayUrl = "Search or enter address";
        drawTextSingleLine(renderer, displayUrl, (int)textX, (int)textY, {120, 120, 130, 255}, 14);
    } else {
        drawTextSingleLine(renderer, displayUrl, (int)(textX - g_urlScrollOffset), (int)textY, {230, 230, 235, 255}, 14);

        // Render cursor if focused
        if (focused && (SDL_GetTicks() / 500) % 2 == 0) {
            SDL_FRect cursorRect = {textX + (float)cursorX - g_urlScrollOffset, textY + 1.0f, 2.0f, 16.0f};
            SDL_SetRenderDrawColor(renderer, 100, 150, 255, 255);
            SDL_RenderFillRect(renderer, &cursorRect);
        }
    }
    
    // Restore viewport
    SDL_SetRenderViewport(renderer, &prevViewport);
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
    
    auto tabIds = tabManager.getAllTabIds();
    
    // Calculate total tab width and max scroll
    int totalTabWidth = 16 + (int)tabIds.size() * (TAB_WIDTH + TAB_SPACING) + 40; 
    g_maxTabScroll = std::max(0.0f, (float)totalTabWidth - (float)windowWidth);
    if (g_tabScrollOffset > g_maxTabScroll) g_tabScrollOffset = g_maxTabScroll;

    // Set clipping for tab bar
    SDL_Rect tabViewport = {0, 0, windowWidth, TAB_HEIGHT};
    SDL_Rect prevViewport;
    SDL_GetRenderViewport(renderer, &prevViewport);
    SDL_SetRenderViewport(renderer, &tabViewport);

    int x = 8 - (int)g_tabScrollOffset;
    
    for (int tabId : tabIds) {
        Tab* tab = tabManager.getTab(tabId);
        if (!tab) continue;
        
        bool isActive = (tabId == activeTabId);
        
        float tabX = static_cast<float>(x);
        float tabY = 8.0f; // Standard top margin
        float tabW = static_cast<float>(TAB_WIDTH);
        float tabH = static_cast<float>(TAB_HEIGHT - 6);
        
        SDL_FRect tabRect = {tabX, tabY, tabW, tabH};
        
        // Tab shape: Squared edges, merged pass
        SDL_Color tabBg = isActive ? SDL_Color{30, 31, 34, 255} : SDL_Color{40, 42, 45, 255};
        SDL_Color tabBorder = isActive ? SDL_Color{100, 150, 255, 255} : SDL_Color{50, 52, 55, 255};
        
        drawSquaredBox(renderer, tabX, tabY, tabW, tabH, tabBg, tabBorder, true);
        
        if (isActive) {
            // Precise top accent
            SDL_FRect accent = {tabX, tabY, tabW, 3};
            SDL_SetRenderDrawColor(renderer, 100, 150, 255, 255);
            SDL_RenderFillRect(renderer, &accent);
        }
        
        // Label
        std::string label = tab->getUrl();
        if (label.empty()) label = "New Tab";
        else {
            size_t lastSlash = label.find_last_of("/\\");
            if (lastSlash != std::string::npos && lastSlash + 1 < label.length()) {
                label = label.substr(lastSlash + 1);
            }
        }
        if (label.length() > 20) label = label.substr(0, 17) + "...";
        
        SDL_Color textColor = isActive ? SDL_Color{230, 230, 235, 255} : SDL_Color{140, 145, 150, 255};
        drawText(renderer, label, (int)tabX + 12, (int)tabY + 10, TAB_WIDTH - 40, textColor, 13);
        
        // Close button (Merged square component)
        float cx = tabX + tabW - 24;
        float cy = tabY + (tabH - CLOSE_BUTTON_SIZE) / 2;
        SDL_FRect closeRect = {cx, cy, (float)CLOSE_BUTTON_SIZE, (float)CLOSE_BUTTON_SIZE};
        g_closeRects.push_back({tabId, closeRect});
        
        if (isActive) {
            drawSquaredBox(renderer, cx - 2, cy - 2, CLOSE_BUTTON_SIZE + 4, CLOSE_BUTTON_SIZE + 4, {60, 63, 66, 255}, {80, 83, 86, 255}, false);
        }
        
        SDL_Point xSize = measureText("x", 12);
        float xOff = (CLOSE_BUTTON_SIZE - xSize.x) / 2.0f;
        float yOff = (CLOSE_BUTTON_SIZE - xSize.y) / 2.0f;
        drawText(renderer, "x", (int)cx + (int)xOff - 1, (int)cy + (int)yOff - 2, CLOSE_BUTTON_SIZE, {180, 180, 180, 255}, 10);
        
        g_tabRects.push_back({tabId, tabRect});
        x += TAB_WIDTH + TAB_SPACING;
    }
    
    // "+" button (Squared)
    float plusX = (float)x + 10;
    float plusY = 12.0f;
    float plusSize = 24.0f;
    SDL_FRect plusRect = {plusX, plusY, plusSize, plusSize};
    
    drawSquaredBox(renderer, plusX, plusY, plusSize, plusSize, {60, 63, 66, 255}, {80, 83, 86, 255}, true);
    drawText(renderer, "+", (int)plusX + 7, (int)plusY + 1, (int)plusSize, {220, 220, 225, 255}, 18);
    
    g_tabRects.push_back({-99, plusRect}); 
    
    // Reset viewport
    SDL_SetRenderViewport(renderer, &prevViewport);

    // Render Tab Scrollbar if overflowing
    if (g_maxTabScroll > 0) {
        float ratio = (float)windowWidth / (float)totalTabWidth;
        float barW = std::max(20.0f, ratio * (float)windowWidth);
        float barX = (g_tabScrollOffset / g_maxTabScroll) * ((float)windowWidth - barW);
        
        SDL_FRect tabScrollRect = {barX, (float)TAB_HEIGHT - 3.0f, barW, 2.0f};
        SDL_SetRenderDrawColor(renderer, 59, 130, 246, 200); // Blue accent
        SDL_RenderFillRect(renderer, &tabScrollRect);
    }
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

void renderDom(SDL_Renderer* renderer, const Node* root, const std::string& baseUrl) {
    if (!renderer) return;

    if (!root) {
        // Render a minimalist squared welcome screen
        SDL_FRect viewport = {0, 90, 1280, 630}; 
        SDL_SetRenderDrawColor(renderer, 18, 18, 18, 255);
        SDL_RenderFillRect(renderer, &viewport);
        
        // Centered Welcome Text
        std::string welcome = "Welcome to Technitium Aether";
        std::string subtext = "Type a URL or file path in the address bar above to begin.";
        
        SDL_Point wSize = measureText(welcome, 28);
        SDL_Point sSize = measureText(subtext, 14);
        
        int centerX = 1280 / 2;
        int centerY = 90 + (630 / 2);
        
        drawText(renderer, welcome, centerX - (wSize.x / 2), centerY - 40, 800, {245, 245, 250, 255}, 28);
        drawText(renderer, subtext, centerX - (sSize.x / 2), centerY + 10, 800, {150, 150, 160, 255}, 14);
        return;
    }
    renderNode(renderer, root, baseUrl);
}

void renderScrollbar(SDL_Renderer* renderer, float scrollY, float maxScrollY, int viewportHeight, int windowWidth, int viewportY) {
    if (maxScrollY <= 0) return;
    
    float totalHeight = (float)viewportHeight + maxScrollY;
    float ratio = (float)viewportHeight / totalHeight;
    float handleHeight = std::max(40.0f, ratio * (float)viewportHeight);
    float handlePos = (scrollY / maxScrollY) * ((float)viewportHeight - handleHeight);
    
    SDL_FRect track = { (float)windowWidth - 14.0f, (float)viewportY, 14.0f, (float)viewportHeight };
    SDL_FRect handle = { (float)windowWidth - 11.0f, (float)viewportY + handlePos, 8.0f, handleHeight };
    
    // Draw track (subtle dark gray background with border)
    SDL_SetRenderDrawColor(renderer, 24, 25, 30, 160);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_RenderFillRect(renderer, &track);
    
    // Draw handle (Vibrant accent blue for high visibility)
    SDL_SetRenderDrawColor(renderer, 59, 130, 246, 220); // #3b82f6 blue
    SDL_RenderFillRect(renderer, &handle);
    
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}
