#include <SDL3/SDL.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

#include "dom.hpp"
#include "layout.hpp"
#include "network.hpp"
#include "renderer.hpp"
#include "scripting.hpp"

static const char* defaultMarkup = R"JSON(
{
  "lua": "",
  "root": {
    "flexv": [
      {
         "text": { "content": "Technitium Toy Browser", "fontsize": 24, "color": "#ffffff" },
         "tag" : "title",
         "bgcolor": "#1a1b26",
         "spacing": 10
      },
      {
         "text": { "content": "Run ./browser assets/index.jsml to see the interactive Lua components.", "fontsize": 14, "color": "#aaaaaa" },
         "tag" : "info",
         "bgcolor": "#24283b",
         "spacing": 5
      }
    ]
  }
}
)JSON";

static bool loadFileCustom(const std::string& path, std::string& out) {
    if (path.empty()) return false;
    std::ifstream t(path);
    if (!t.is_open()) return false;
    std::stringstream buffer;
    buffer << t.rdbuf();
    out = buffer.str();
    return true;
}

static Node* getIntersectingNode(Node* current, float mouseX, float mouseY) {
    if (!current) return nullptr;
    
    // Check children first (drawn on top)
    for (auto it = current->children.rbegin(); it != current->children.rend(); ++it) {
        Node* hit = getIntersectingNode(*it, mouseX, mouseY);
        if (hit) return hit;
    }

    if (mouseX >= current->x && mouseX <= current->x + current->width &&
        mouseY >= current->y && mouseY <= current->y + current->height) {
        return current;
    }
    
    return nullptr;
}

static void navigateTo(const std::string& url, Node** document, std::string& currentUrl) {
    std::string pageSource;
    if (url.rfind("http://", 0) == 0 || url.rfind("https://", 0) == 0) {
        if (!fetchUrl(url, pageSource)) {
            std::cerr << "Failed to fetch URL: " << url << std::endl;
            return;
        }
    } else {
        if (!loadFileCustom(url, pageSource)) {
            std::cerr << "Failed to load file: " << url << std::endl;
            return;
        }
    }

    Node* newDoc = parseMarkup(pageSource);
    if (!newDoc) {
        std::cerr << "Failed to parse JSML from: " << url << std::endl;
        return;
    }

    if (*document) {
        destroyDom(*document);
    }
    *document = newDoc;
    currentUrl = url;

    shutdownScripting();
    if (!initScripting(*document)) {
        std::cerr << "Lua scripting re-initialization failed." << std::endl;
    }
    setCurrentUrl(currentUrl);

    if (newDoc->properties.contains("lua") && newDoc->properties["lua"].is_string()) {
        std::string scriptPath = newDoc->properties["lua"].get<std::string>();
        if (!scriptPath.empty()) {
            std::string code;
            if (loadFileCustom(scriptPath, code)) {
                runScript(code);
            }
        }
    }
}

int main(int argc, char* argv[]) {
    std::string pageSource;
    std::string currentUrl = "default";
    std::string urlInput = "";
    bool urlBoxFocused = false;

    if (argc > 1) {
        std::string target = argv[1];
        urlInput = target;
        if (target.rfind("http://", 0) == 0 || target.rfind("https://", 0) == 0) {
            fetchUrl(target, pageSource);
            currentUrl = target;
        } else if (loadFileCustom(target, pageSource)) {
            currentUrl = target;
        }
    }

    if (pageSource.empty()) {
        pageSource = defaultMarkup;
    }

    Node* document = parseMarkup(pageSource);
    if (!document) {
        std::cerr << "Failed to parse initial page content." << std::endl;
        return EXIT_FAILURE;
    }

    if (!initScripting(document)) {
        std::cerr << "Lua scripting initialization failed." << std::endl;
    }
    setCurrentUrl(currentUrl);
    setScreenSize(1280, 720);

    // Evaluate external JSML lua source
    if (document->properties.contains("lua") && document->properties["lua"].is_string()) {
        std::string scriptPath = document->properties["lua"].get<std::string>();
        if (!scriptPath.empty()) {
            std::string code;
            if (loadFileCustom(scriptPath, code)) {
                runScript(code);
            }
        }
    }

    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    if (!initRenderer(&window, &renderer)) {
        std::cerr << "Failed to initialize SDL renderer." << std::endl;
        destroyDom(document);
        shutdownScripting();
        return EXIT_FAILURE;
    }

    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }
            if (event.type == SDL_EVENT_KEY_DOWN) {
                if (event.key.key == SDLK_ESCAPE) {
                    running = false;
                } else if (urlBoxFocused) {
                    if (event.key.key == SDLK_BACKSPACE && !urlInput.empty()) {
                        urlInput.pop_back();
                    } else if (event.key.key == SDLK_RETURN || event.key.key == SDLK_KP_ENTER) {
                        navigateTo(urlInput, &document, currentUrl);
                        urlBoxFocused = false;
                        SDL_StopTextInput(window);
                    }
                }
            }
            if (event.type == SDL_EVENT_TEXT_INPUT && urlBoxFocused) {
                urlInput += event.text.text;
            }
            if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
                float mx = event.button.x;
                float my = event.button.y;
                
                if (my < 40) {
                    urlBoxFocused = true;
                    SDL_StartTextInput(window);
                } else {
                    urlBoxFocused = false;
                    SDL_StopTextInput(window);
                    Node* hitTarget = getIntersectingNode(document, mx, my);
                    if (hitTarget) {
                        triggerScriptClick(hitTarget->id);
                    }
                }
            }
        }

        // Must re-layout before render in case updateElem was called
        // Content area starts at y=40
        layoutNode(document, 0.0f, 40.0f, 1280.0f, 680.0f);

        SDL_SetRenderDrawColor(renderer, 14, 16, 22, 255);
        SDL_RenderClear(renderer);
        
        renderDom(renderer, document);
        renderUrlBox(renderer, urlInput, urlBoxFocused, 1280, 40);
        
        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    destroyDom(document);
    shutdownRenderer();
    shutdownScripting();
    return EXIT_SUCCESS;
}
