#include <SDL3/SDL.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <algorithm>

#include "dom.hpp"
#include "layout.hpp"
#include "network.hpp"
#include "renderer.hpp"
#include "scripting.hpp"
#include "tab.hpp"

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

// Redundant function removed as plus button is now tracked in g_tabRects

int main(int argc, char* argv[]) {
    // Initialize TabManager
    TabManager tabManager;
    
    // Determine initial URL
    std::string initialUrl = "assets/index.jsml";
    if (argc > 1) {
        initialUrl = argv[1];
    }
    
    // Create first tab
    Tab* activeTab = tabManager.createTab(initialUrl);
    if (!activeTab) {
        std::cerr << "Failed to create initial tab" << std::endl;
        return EXIT_FAILURE;
    }
    
    // Load content into first tab
    if (!tabManager.loadTab(activeTab, initialUrl)) {
        std::cerr << "Failed to load initial content" << std::endl;
    }
    
    // URL bar state
    std::string urlInput = initialUrl;
    int urlCursorPos = static_cast<int>(urlInput.size());
    bool urlBoxFocused = false;
    
    // Initialize renderer
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    if (!initRenderer(&window, &renderer)) {
        std::cerr << "Failed to initialize SDL renderer." << std::endl;
        tabManager.closeAllTabs();
        return EXIT_FAILURE;
    }
    
    const int WINDOW_WIDTH = 1280;
    const int WINDOW_HEIGHT = 720;
    const int TAB_BAR_HEIGHT = 42; // Updated from 36
    const int URL_BAR_HEIGHT = 48; // Updated from 44
    const int CHROME_HEIGHT = TAB_BAR_HEIGHT + URL_BAR_HEIGHT;
    const int VIEWPORT_Y = CHROME_HEIGHT;
    const int VIEWPORT_HEIGHT = WINDOW_HEIGHT - CHROME_HEIGHT;
    
    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }
            
            // Keyboard handling
            if (event.type == SDL_EVENT_KEY_DOWN) {
                // Global shortcuts (work even if URL bar focused)
                bool isCtrl = (event.key.mod & SDL_KMOD_CTRL) != 0;
                
                if (isCtrl) {
                    if (event.key.key == SDLK_T) {
                        // Ctrl+T: New tab
                        Tab* newTab = tabManager.createTab("assets/index.jsml");
                        if (newTab) {
                            tabManager.loadTab(newTab, "assets/index.jsml");
                            activeTab = newTab;
                            urlInput = "assets/index.jsml";
                            urlCursorPos = static_cast<int>(urlInput.size());
                            urlBoxFocused = false;
                            if (window) SDL_StopTextInput(window);
                        }
                        continue;
                    } else if (event.key.key == SDLK_W) {
                        // Ctrl+W: Close current tab
                        if (activeTab) {
                            int closeId = activeTab->getId();
                            tabManager.closeTab(closeId);
                            activeTab = tabManager.getActiveTab();
                            if (activeTab) {
                                urlInput = activeTab->getUrl();
                                urlCursorPos = static_cast<int>(urlInput.size());
                            }
                            urlBoxFocused = false;
                            if (window) SDL_StopTextInput(window);
                        }
                        continue;
                    } else if (event.key.key == SDLK_TAB) {
                        // Ctrl+Tab: Switch to next tab
                        auto ids = tabManager.getAllTabIds();
                        if (ids.size() > 1) {
                            for (size_t i = 0; i < ids.size(); i++) {
                                if (ids[i] == activeTab->getId()) {
                                    int nextIdx = (i + 1) % ids.size();
                                    tabManager.switchTab(ids[nextIdx]);
                                    activeTab = tabManager.getActiveTab();
                                    if (activeTab) {
                                        urlInput = activeTab->getUrl();
                                        urlCursorPos = static_cast<int>(urlInput.size());
                                    }
                                    break;
                                }
                            }
                        }
                        urlBoxFocused = false;
                        if (window) SDL_StopTextInput(window);
                        continue;
                    }
                }
                
                // ESC key
                if (event.key.key == SDLK_ESCAPE) {
                    running = false;
                } else if (urlBoxFocused) {
                    // URL bar focused - handle text editing
                    if (isCtrl && event.key.key == SDLK_V) {
                        char* clip = SDL_GetClipboardText();
                        if (clip) {
                            std::string pasted(clip);
                            SDL_free(clip);
                            if (!pasted.empty()) {
                                urlInput.insert(urlCursorPos, pasted);
                                urlCursorPos += static_cast<int>(pasted.size());
                            }
                        }
                    } else if (event.key.key == SDLK_BACKSPACE) {
                        if (urlCursorPos > 0 && !urlInput.empty()) {
                            urlInput.erase(urlCursorPos - 1, 1);
                            urlCursorPos = std::max(0, urlCursorPos - 1);
                        }
                    } else if (event.key.key == SDLK_DELETE) {
                        if (urlCursorPos < static_cast<int>(urlInput.size())) {
                            urlInput.erase(urlCursorPos, 1);
                        }
                    } else if (event.key.key == SDLK_LEFT) {
                        urlCursorPos = std::max(0, urlCursorPos - 1);
                    } else if (event.key.key == SDLK_RIGHT) {
                        urlCursorPos = std::min(static_cast<int>(urlInput.size()), urlCursorPos + 1);
                    } else if (event.key.key == SDLK_HOME) {
                        urlCursorPos = 0;
                    } else if (event.key.key == SDLK_END) {
                        urlCursorPos = static_cast<int>(urlInput.size());
                    } else if (event.key.key == SDLK_RETURN || event.key.key == SDLK_KP_ENTER) {
                        // Navigate to URL
                        if (activeTab && !urlInput.empty()) {
                            tabManager.navigateTab(activeTab, urlInput);
                        }
                        urlBoxFocused = false;
                        if (window) SDL_StopTextInput(window);
                    }

                    if (urlCursorPos > static_cast<int>(urlInput.size())) {
                        urlCursorPos = static_cast<int>(urlInput.size());
                    }
                }
            }
            
            // Text input (when URL bar focused)
            if (event.type == SDL_EVENT_TEXT_INPUT && urlBoxFocused) {
                std::string text(event.text.text);
                if (!text.empty()) {
                    urlInput.insert(urlCursorPos, text);
                    urlCursorPos += static_cast<int>(text.length());
                }
            }
            
            // Mouse click handling
            if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
                float mx = event.button.x;
                float my = event.button.y;

                if (my < CHROME_HEIGHT) {
                    // Click on chrome area (tabs or URL bar)
                    if (my < TAB_BAR_HEIGHT) {
                        // Check for close button click first
                        int closeId = getCloseButtonTabAtPosition(mx, my);
                        if (closeId >= 0) {
                            tabManager.closeTab(closeId);
                            activeTab = tabManager.getActiveTab();
                            if (activeTab) {
                                urlInput = activeTab->getUrl();
                                urlCursorPos = static_cast<int>(urlInput.size());
                            }
                        } else {
                            // Tab bar click - check which tab was clicked
                            int clickedTabId = getTabAtPosition(mx, my);
                            if (clickedTabId == -99) {
                                // Special ID for plus button
                                Tab* newTab = tabManager.createTab("assets/index.jsml");
                                if (newTab) {
                                    tabManager.loadTab(newTab, "assets/index.jsml");
                                    activeTab = newTab;
                                    urlInput = "assets/index.jsml";
                                    urlCursorPos = static_cast<int>(urlInput.size());
                                }
                            } else if (clickedTabId >= 0) {
                                // Switch to clicked tab
                                tabManager.switchTab(clickedTabId);
                                activeTab = tabManager.getActiveTab();
                                if (activeTab) {
                                    urlInput = activeTab->getUrl();
                                    urlCursorPos = static_cast<int>(urlInput.size());
                                }
                            }
                        }
                        urlBoxFocused = false;
                        if (window) SDL_StopTextInput(window);
                    } else {
                        // URL bar area click
                        urlBoxFocused = true;
                        if (window) SDL_StartTextInput(window);

                        int relativeX = static_cast<int>(mx - 100); // 80 (boxX) + 20 (padding)
                        if (relativeX <= 0) {
                            urlCursorPos = 0;
                        } else {
                            urlCursorPos = static_cast<int>(urlInput.size());
                            int closest = 0;
                            for (int i = 0; i <= static_cast<int>(urlInput.size()); ++i) {
                                SDL_Point sz = measureText(urlInput.substr(0, i), 14);
                                if (sz.x > relativeX) {
                                    break;
                                }
                                closest = i;
                            }
                            urlCursorPos = closest;
                        }
                    }
                } else {
                    // Click in viewport (content area)
                    urlBoxFocused = false;
                    if (window) SDL_StopTextInput(window);
                    
                    if (activeTab && activeTab->getDomRoot()) {
                        // Coordinates are already correct since DOM is laid out at VIEWPORT_Y
                        Node* hitTarget = getIntersectingNode(activeTab->getDomRoot(), mx, my);
                        if (hitTarget) {
                            triggerTabScriptClick(activeTab, hitTarget->id);
                        }
                    }
                }
            }
        }

        // Layout the DOM for active tab
        if (activeTab && activeTab->getDomRoot()) {
            layoutNode(activeTab->getDomRoot(), 0.0f, static_cast<float>(VIEWPORT_Y), 
                      static_cast<float>(WINDOW_WIDTH), 
                      static_cast<float>(VIEWPORT_HEIGHT));
        }

        // Render
        SDL_SetRenderDrawColor(renderer, 14, 16, 22, 255);
        SDL_RenderClear(renderer);
        
        // Render tab bar
        renderTabBar(renderer, tabManager, activeTab ? activeTab->getId() : -1, WINDOW_WIDTH);
        
        // Render URL box (positioned below tab bar)
        renderUrlBox(renderer, urlInput, urlBoxFocused, WINDOW_WIDTH, URL_BAR_HEIGHT, urlCursorPos, TAB_BAR_HEIGHT);
        
        // Render active tab's DOM
        if (activeTab && activeTab->getDomRoot()) {
            renderDom(renderer, activeTab->getDomRoot());
        }
        
        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    // Cleanup
    tabManager.closeAllTabs();
    shutdownRenderer();
    return EXIT_SUCCESS;
}
