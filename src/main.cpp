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

int main(int argc, char* argv[]) {
    std::string pageSource;
    std::string currentUrl = "default";
    if (argc > 1) {
        std::string target = argv[1];
        currentUrl = target;
        if (target.rfind("http://", 0) == 0 || target.rfind("https://", 0) == 0) {
            // fetchUrl is stubbed somewhere else or not fully available
            // but we'll try it if provided
        } else if (!loadFileCustom(target, pageSource)) {
            std::cerr << "Unable to load file: " << target << std::endl;
        }
    }

    if (pageSource.empty()) {
        pageSource = defaultMarkup;
    }

    Node* document = parseMarkup(pageSource);
    if (!document) {
        std::cerr << "Failed to parse page content." << std::endl;
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
            } else {
                std::cerr << "Could not load JSML linked script: " << scriptPath << std::endl;
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
            if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE) {
                running = false;
            }
            if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
                float mx = event.button.x;
                float my = event.button.y;
                Node* hitTarget = getIntersectingNode(document, mx, my);
                if (hitTarget) {
                    triggerScriptClick(hitTarget->id);
                }
            }
        }

        // Must re-layout before render in case updateElem was called
        layoutNode(document, 0.0f, 0.0f, 1280.0f, 720.0f);

        SDL_SetRenderDrawColor(renderer, 14, 16, 22, 255);
        SDL_RenderClear(renderer);
        renderDom(renderer, document);
        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    destroyDom(document);
    shutdownRenderer();
    shutdownScripting();
    return EXIT_SUCCESS;
}
