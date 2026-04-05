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
    Node* domRoot;  // Private DOM tree for this tab
    std::unique_ptr<sol::state> luaState;  // Isolated Lua environment
    TripleBuffer<RenderCommand> renderTripleBuffer;  // Thread-safe render tree passing
    std::map<int, sol::function> clickHandlers;  // Tab-specific click handlers
    
    friend class TabManager;
    
public:
    Tab(int id, const std::string& initialUrl = "");
    ~Tab();
    
    // Getters
    int getId() const { return tabId; }
    const std::string& getUrl() const { return url; }
    Node* getDomRoot() const { return domRoot; }
    sol::state* getLuaState() { return luaState.get(); }
    TripleBuffer<RenderCommand>& getRenderBuffer() { return renderTripleBuffer; }
    
    // DOM management
    void setDomRoot(Node* newRoot);
    void clearDom();
    
    // URL management
    void setUrl(const std::string& newUrl) { url = newUrl; }
    
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
