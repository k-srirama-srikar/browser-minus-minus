#include <SDL3/SDL.h>
#include <iostream>
#include <string>

#include "dom.hpp"
#include "layout.hpp"
#include "network.hpp"
#include "renderer.hpp"
#include "scripting.hpp"

static const char* defaultMarkup = R"HTML(
<flexv style="padding:16;gap:12">
  <flexh style="gap:12">
    <text>Technitium Toy Browser</text>
    <text>Rendered with SDL3, CURL, and Lua</text>
  </flexh>
  <flexh style="gap:10">
    <image src="placeholder" width="280" height="160"/>
    <flexv style="gap:8">
      <text>Welcome to a toy browser shell.</text>
      <text>Open a local file or URL using:</text>
      <text>./browser https://example.com\</text\>
      <text>Or use a tiny markup page with &lt;flexv&gt;/&lt;flexh&gt;/&lt;text&gt; tags.</text>
    </flexv>
  </flexh>
</flexv>
)HTML";

static void runScripts(Node* node) {
    if (!node) {
        return;
    }
    auto it = node->attrs.find("script");
    if (it != node->attrs.end() && !it->second.empty()) {
        runScript(it->second);
    }
    for (Node* child : node->children) {
        runScripts(child);
    }
}

int main(int argc, char* argv[]) {
    std::string pageSource;
    if (argc > 1) {
        std::string target = argv[1];
        if (target.rfind("http://", 0) == 0 || target.rfind("https://", 0) == 0) {
            if (!fetchUrl(target, pageSource)) {
                std::cerr << "Unable to fetch URL: " << target << std::endl;
            }
        } else if (!loadFile(target, pageSource)) {
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
    runScripts(document);

    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    if (!initRenderer(&window, &renderer)) {
        std::cerr << "Failed to initialize SDL renderer." << std::endl;
        destroyDom(document);
        shutdownScripting();
        return EXIT_FAILURE;
    }

    layoutNode(document, 0.0f, 0.0f, 1280.0f, 720.0f);

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
        }

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
