#pragma once

#include <string>
#include <vector>
#include <memory>
#include <map>
#include <sol/sol.hpp>
#include "dom.hpp"
#include "buffer.hpp"

class Tab {
private:
    int tabId;
    std::string url;
    std::string baseUrl;
    Node* domRoot;  // Private DOM tree for this tab
    std::unique_ptr<sol::state> luaState;  // Isolated Lua environment
    TripleBuffer<RenderCommand> renderTripleBuffer;  // Thread-safe render tree passing
    std::map<int, sol::function> clickHandlers;  // Tab-specific click handlers
    
    // Scrolling state
    float scrollY = 0.0f;
    float maxScrollY = 0.0f;
    
    friend class TabManager;
    
public:
    Tab(int id, const std::string& initialUrl = "");
    ~Tab();
    
    // Getters
    int getId() const { return tabId; }
    const std::string& getUrl() const { return url; }
    const std::string& getBaseUrl() const { return baseUrl; }
    Node* getDomRoot() const { return domRoot; }
    sol::state* getLuaState() { return luaState.get(); }
    TripleBuffer<RenderCommand>& getRenderBuffer() { return renderTripleBuffer; }
    
    // Scrolling
    float getScrollY() const { return scrollY; }
    float getMaxScrollY() const { return maxScrollY; }
    void setScrollY(float y) { 
        scrollY = std::max(0.0f, std::min(y, maxScrollY)); 
    }
    void setMaxScrollY(float max) { maxScrollY = std::max(0.0f, max); }
    void handleScroll(float delta) {
        setScrollY(scrollY + delta);
    }
    
    // DOM management
    void setDomRoot(Node* newRoot);
    void clearDom();
    
    // URL management
    void setUrl(const std::string& newUrl) { url = newUrl; }
    void setBaseUrl(const std::string& newBase) { baseUrl = newBase; }
    
    // Lua integration
    bool initializeLua();
    void shutdownLua();
    void runScript(const std::string& code);
    void triggerClick(int nodeId);
    
    // Render buffer
    std::vector<RenderCommand>& getBackRenderBuffer() {
        return renderTripleBuffer.getBackBuffer();
    }
    
    void swapRenderBuffers() {
        renderTripleBuffer.swapBuffers();
    }
    
    const std::vector<RenderCommand>& getFrontRenderBuffer() const {
        return renderTripleBuffer.getFrontBuffer();
    }
    
    // Click handler management
    void registerClickHandler(int nodeId, sol::function handler) {
        clickHandlers[nodeId] = handler;
    }
    
    sol::function* getClickHandler(int nodeId) {
        auto it = clickHandlers.find(nodeId);
        return it != clickHandlers.end() ? &it->second : nullptr;
    }
};

// TabManager: Central manager for all tabs
class TabManager {
private:
    std::vector<std::unique_ptr<Tab>> tabs;
    int activeTabId;
    int nextTabId = 0;
    static const int MAX_TABS = 32;
    
public:
    TabManager();
    ~TabManager();
    
    // Tab lifecycle
    Tab* createTab(const std::string& url = "");
    bool closeTab(int tabId);
    Tab* getTab(int tabId);
    Tab* getActiveTab();
    bool switchTab(int tabId);
    
    // Tab queries
    int getActiveTabId() const { return activeTabId; }
    size_t getTabCount() const { return tabs.size(); }
    std::vector<int> getAllTabIds() const;
    
    // Tab management and navigation
    bool loadTab(Tab* tab, const std::string& url);
    void navigateTab(Tab* tab, const std::string& url);
    
    // Cleanup
    void closeAllTabs();
};
