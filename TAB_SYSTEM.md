# Tab System Architecture

## Overview

The Tab system implements **fully isolated browsing contexts** for the Technitium browser. Each tab has:

1. **Isolated DOM Tree** - Private `Node*` tree structure per tab
2. **Isolated Lua Environment** - Each tab has its own `sol::state` to prevent script pollution
3. **Isolated Render State** - Separate render commands per tab via TripleBuffer
4. **Thread-Safe Rendering** - TripleBuffer ensures safe DOM→Render thread communication

## Core Components

### 1. TripleBuffer<T> (buffer.hpp)

A lock-free, thread-safe data structure for passing render data from the Logic thread to the Main/Render thread using three rotating buffers:

```cpp
// Back buffer (written by Logic thread)
std::vector<RenderCommand>& backBuf = tab->getBackRenderBuffer();

// Update render commands...
backBuf.clear();
backBuf.push_back({RenderCommand::DrawText, ...});

// Swap when done (makes new data available)
tab->swapRenderBuffers();

// Front buffer (read by Render thread - always consistent)
const auto& frontBuf = tab->getFrontRenderBuffer();
for (const auto& cmd : frontBuf) {
    // Render...
}
```

**Buffer Rotation:**
```
Back  → Mid   → Front → Back (cycle)
Write   Ready    Read    Write
```

### 2. Tab Class (tab.hpp)

Encapsulates a single browsing context:

```cpp
class Tab {
private:
    int tabId;                                      // Unique identifier
    std::string url;                                // Current URL/path
    Node* domRoot;                                  // DOM tree root
    std::unique_ptr<sol::state> luaState;          // Isolated Lua
    TripleBuffer<RenderCommand> renderTripleBuffer; // Thread-safe rendering
    std::map<int, sol::function> clickHandlers;    // Event handlers
};
```

**Key Methods:**

| Method | Purpose |
|--------|---------|
| `getId()` | Get unique tab ID |
| `getUrl()` | Get current URL |
| `getDomRoot()` | Access DOM tree |
| `getLuaState()` | Access Lua environment |
| `setDomRoot(newRoot)` | Replace page content |
| `runScript(code)` | Execute Lua in tab's isolated environment |
| `triggerClick(nodeId)` | Call tab-specific click handlers |

### 3. RenderCommand (buffer.hpp)

Represents a single rendering instruction:

```cpp
struct RenderCommand {
    enum Type { DrawText, DrawRect, DrawImage, Clear } type;
    float x, y, width, height;
    std::string content;
    std::string color;
    std::string imagePath;
    int fontSize;
};
```

### 4. TabManager (tab.hpp)

Global manager for all browser tabs:

```cpp
class TabManager {
public:
    Tab* createTab(const std::string& url = "");
    bool closeTab(int tabId);
    Tab* getTab(int tabId);
    Tab* getActiveTab();
    bool switchTab(int tabId);
    size_t getTabCount() const;
};
```

## Isolated Lua Environments

Each tab's Lua state is **completely isolated**. No global variables leak between tabs:

```lua
-- Tab A
browser.on(5, "click", function()
    local myVar = "I only exist in Tab A"
end)

-- Tab B (different Lua state)
-- myVar is NOT accessible here
```

### Browser API per Tab

Each tab's Lua environment has access to:

```lua
-- Tab-local browser API
browser.log(msg)                    -- Log message
browser.getTime()                   -- Get current timestamp
browser.getCurrentUrl()             -- Get tab's URL
browser.getScreenSize()             -- Returns {width, height}
browser.getElemRect(id)             -- Get element layout
browser.getElem(id)                 -- Get element properties
browser.getElemsByTag(tag)          -- Find elements by tag
browser.setElemText(id, text)       -- Update text content
browser.setElemProp(id, prop, val)  -- Update element property
browser.fetch(url, callback)        -- Load from URL/path
browser.on(nodeId, "click", fn)     -- Register click handler
```

## Architecture: Main Loop Integration

```
Single Main Thread Rendering Loop:
├─ Handle tab navigation (create/close/switch)
├─ For active tab:
│  ├─ Poll input events
│  ├─ Update URL bar if needed
│  ├─ Re-layout DOM (if needed)
│  └─ Read front buffer and render
└─ Poll network callbacks (asynchronously loading resources)

Logic Thread (Future: Threading Support):
├─ For each tab in background:
│  ├─ Run pending Lua scripts
│  ├─ Update DOM
│  ├─ Layout to RenderCommand tree
│  └─ Swap render buffers
└─ Handle async network loads
```

## Usage Example

### Creating and Managing Tabs

```cpp
// Create tab manager
TabManager tabs;

// Create first tab
Tab* tab1 = tabs.createTab("assets/index.jsml");
if (tab1) {
    tab1->initializeLua();  // Setup Lua environment
    
    // Navigate within tab
    std::string pageSource;
    if (loadFile("assets/test.jsml", pageSource)) {
        Node* newRoot = parseMarkup(pageSource);
        tab1->setDomRoot(newRoot);
        tab1->runScript("-- Lua code here");
    }
}

// Create second tab
Tab* tab2 = tabs.createTab("https://example.com");

// Switch between tabs
tabs.switchTab(tab1->getId());
Tab* active = tabs.getActiveTab();

// Render active tab each frame
const auto& renderCommands = active->getFrontRenderBuffer();
for (const auto& cmd : renderCommands) {
    // Render command...
}
```

### Handling Navigation

```cpp
void navigateTab(Tab* tab, const std::string& url) {
    std::string pageSource;
    
    // Load from URL or local path
    if (url.rfind("http://", 0) == 0 || url.rfind("https://", 0) == 0) {
        if (!fetchUrl(url, pageSource)) return;
    } else {
        if (!loadFile(url, pageSource)) return;
    }
    
    // Parse and set new DOM
    Node* newDoc = parseMarkup(pageSource);
    if (newDoc) {
        tab->setDomRoot(newDoc);
        tab->setUrl(url);
        
        // Re-initialize Lua for new page
        tab->shutdownLua();
        tab->initializeLua();
    }
}
```

### UI Rendering Loop

```cpp
int main() {
    TabManager tabManager;
    Tab* activeTab = tabManager.createTab("assets/index.jsml");
    
    while (running) {
        // Input
        float mouseX, mouseY;
        bool clicked = pollMouse(&mouseX, &mouseY);
        
        if (clicked && currentTab && currentTab->getDomRoot()) {
            Node* hitNode = getIntersectingNode(
                currentTab->getDomRoot(), mouseX, mouseY);
            if (hitNode) {
                triggerTabScriptClick(currentTab, hitNode->id);
            }
        }
        
        // Layout
        if (currentTab && currentTab->getDomRoot()) {
            layoutNode(currentTab->getDomRoot(), 0, 40, 1280, 680);
        }
        
        // Render
        renderUrlBar(renderer, /* ... */);
        if (currentTab) {
            renderDom(renderer, currentTab->getDomRoot());
        }
        
        SDL_RenderPresent(renderer);
        SDL_Delay(16);  // ~60 FPS
    }
}
```

## Thread Safety

### TripleBuffer Synchronization

The TripleBuffer ensures **no locks** during actual rendering:

```
Logic Thread               Main/Render Thread
┌──────────────────┐      ┌──────────────────┐
│ Write back[0]    │      │ Read front[2]    │
│ (no lock needed) │      │ (no lock needed) │
└────────┬─────────┘      └──────────────────┘
         │
         │ tab->swapRenderBuffers()
         │ (lock only during swap)
         │
         └─→ back[0] → mid[1]
             front[2] ↓
         ┌─→ back[2] ← ← ← ← ← ← ← → → → → →
```

### DOM Thread Safety

Currently DOM operations are **not thread-safe**. Future implementations will need:
- A DOM mutation queue (logic thread queues changes)
- DOM layout calculations on logic thread
- RenderCommand generation then swapped to main thread

## Extending the Tab System

### Adding New Browser APIs

```cpp
// In Tab::initializeLua() browser table setup:
browser.set_function("myNewApi", [thisTab](int param) {
    // This function only operates on thisTab's DOM
    Node* node = findNodeByIdRecursive(thisTab->domRoot, param);
    if (node) {
        // ...
    }
});
```

### Custom Event Handlers

```cpp
// In Lua
browser.on(elementId, "click", function()
    print("Only this tab's element clicked")
    -- Access tab-local state here
end)

// In C++
triggerTabScriptClick(tab, elementId);  // Calls registered handler
```

## Limitations & Future Work

1. **No multithreaded DOM layout** - Currently all work is single-threaded
   - Plan: Logic thread for DOM manipulation, layout, script execution
   - Main thread only reads front buffer for rendering

2. **No shared resources between tabs** - By design
   - Future: Implement resource cache with reference counting

3. **Limited networking** - Blocking fetch calls
   - Plan: Async networking with callbacks

4. **URL loading** - Supports both local paths and network URLs
   - `/path/to/file.jsml` - Local file
   - `http://example.com` - Network (blocking)
   - `file:///absolute/path` - Absolute path

## Summary

The Tab system provides **true browser isolation** where:
- Each tab's DOM is private
- Each tab's scripts cannot see other tabs' variables  
- Each tab has consistent rendering via TripleBuffer
- Tabs can be independently navigated, closed, or switched

This forms the foundation for a proper multi-tab browser experience!
