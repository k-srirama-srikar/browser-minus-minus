# Tabbed Browser UI - Complete Guide

## Overview

The Technitium browser now features a complete **multi-tab interface** with keyboard shortcuts and a visual tab bar. The window is split into distinct areas:

```
┌─────────────────────────────────────────────────┐
│  Tab Bar (30px)                                 │
│  ┌─Tab 1─┐ ┌─Tab 2─┐ ┌─Tab 3─┐                │
└─────────────────────────────────────────────────┘
┌─────────────────────────────────────────────────┐
│  URL Bar (40px)                                 │
│  [ Enter URL or path...                        ]│
└─────────────────────────────────────────────────┘
┌─────────────────────────────────────────────────┐
│  Viewport (650px)                               │
│                                                 │
│  ┌─JSML Content of Active Tab──────────────────┐
│  │                                              │
│  │  Renders DOM tree from current tab          │
│  │                                              │
│  └──────────────────────────────────────────────┘
│                                                 │
└─────────────────────────────────────────────────┘
```

## Window Layout

| Component | Height | Y Position | Width | Purpose |
|-----------|--------|-----------|-------|---------|
| Tab Bar | 30px | 0 | 1280px | Shows open tabs, allows tab identification |
| URL Bar | 40px | 30 | 1280px | Input for URLs/file paths |
| Viewport | 650px | 70 | 1280px | Renders active tab's JSML content |
| **Total** | **720px** | - | 1280px | Standard browser window |

## Keyboard Shortcuts

### Keyboard Controls

| Shortcut | Action | Context |
|----------|--------|---------|
| **Ctrl+T** | Open new tab | Anytime |
| **Ctrl+W** | Close current tab | Anytime |
| **Ctrl+Tab** | Switch to next tab | Anytime |
| **Escape** | Exit browser | Anytime |
| **Ctrl+V** | Paste URL | URL bar focused |
| **Enter** | Navigate to URL | URL bar focused |
| **Backspace** | Delete character | URL bar focused |
| **Home** | Cursor to start | URL bar focused |
| **End** | Cursor to end | URL bar focused |

## Usage Examples

### Creating a New Tab

```
Press: Ctrl+T
Result:
  • New tab created with default page
  • New tab becomes active
  • URL bar shows "assets/index.jsml"
  • Tab bar displays all open tabs
```

### Navigating Between Tabs

```
Press: Ctrl+Tab (multiple times)
Result:
  • Cycles through open tabs sequentially
  • Active tab highlights lighter in tab bar
  • Viewport updates to show selected tab's content
  • URL bar shows current tab's URL
```

### Closing a Tab

```
With Tab A active, Press: Ctrl+W
Result:
  • Tab A closes
  • Tab B becomes active (or first remaining tab)
  • Viewport switches to Tab B
  • Tab bar updates without Tab A
```

### Entering a URL

```
1. Click on URL bar (or it's already focused)
2. Type URL or file path:
   - "assets/test.jsml" - local file
   - "https://example.com" - web URL
   - "/usr/share/data/file.jsml" - absolute path
3. Press Enter
4. Tab navigates to the new URL
5. Content loads and renders in viewport
```

## Tab Bar UI Details

### Tab Display

Each tab in the tab bar shows:

```
┌────────────────┐
│  assets/te...  │  ← Truncated URL (max 20 chars)
└────────────────┘
 ^                ^
 Background color = lighter if active, darker if inactive
 Border shows visual separation
```

### Tab Bar Colors

| State | Background | Border |
|-------|-----------|--------|
| Active Tab | RGB(45, 50, 70) | RGB(60, 65, 85) |
| Inactive Tab | RGB(30, 33, 45) | RGB(60, 65, 85) |
| Bar Background | RGB(20, 22, 30) | - |

### Tab Information

Tab bar displays:
- **Tab ID** (internal, not shown)
- **URL/Path** (truncated to fit)
- **Visual highlight** indicating active tab
- **Separator line** between tab bar and URL bar

## URL Bar Interaction

### Features

- **Text Editing**: Full cursor navigation (left/right/home/end)
- **Text Input**: Type URLs or file paths directly
- **Clipboard Paste**: Ctrl+V to paste from clipboard
- **Visual Feedback**: 
  - Text is white when focused
  - Cursor position shown with blinking indicator
  - Background highlights when active

### Sample URLs

```
assets/index.jsml           → Load local file
assets/test2.jsml           → Another local file
file:///usr/share/data.jsml → Absolute path
https://example.com         → Web URL (if networking available)
http://localhost:8080       → Local server
```

## Tab Context Isolation

### Key Feature: Fully Isolated State

Each tab maintains completely independent state:

```
Tab A                          Tab B
├─ DOM Tree: nodeA_1          ├─ DOM Tree: nodeB_1
├─ Lua State: luaA            ├─ Lua State: luaB
├─ URL: assets/test.jsml      ├─ URL: assets/index.jsml
└─ Event Handlers: {5, 7}     └─ Event Handlers: {1, 3, 9}

When switching from Tab A to Tab B:
✓ Tab A's Lua state is not destroyed (preserved for next switch)
✓ Tab B's DOM, Lua, and events are completely separate
✓ No leakage of variables/state between tabs
✓ Each tab can have different Lua scripts running
```

## Viewport Rendering

### What Gets Rendered

```
Active Tab's DOM Tree
     ↓
Layout Engine (calculates positions)
     ↓
Render Commands (draw instructions)
     ↓
SDL Renderer (SDL3)
     ↓
Screen Display (in viewport area, y >= 70)
```

### Coordinate System

```
Global Window Coordinates:
┌──────────────────────────────────────────┐
│ (0,0)                              (1280,0)
│  Tab Bar
│  ┌──────────────────────────────────────┐
│ (0,30)                            (1280,30)
│  URL Bar
│  ┌──────────────────────────────────────┐
│ (0,70)  Viewport with DOM coordinates   (1280,70)
│ └──────────────────────────────────────┘
│                                   (1280,720)
└──────────────────────────────────────────┘

Note: DOM coordinates are relative to viewport (0,0) = top-left of viewport
      Mouse clicks adjusted: clickY - 70 = viewport Y coordinate
```

## Mouse Interaction

### Click Zones

```
     ↑ Y < 30: Tab bar (not interactive yet)
     ├─ Y 30-70: URL bar (text input)
     └─ Y ≥ 70: Viewport (DOM interaction)
```

### Click Behavior

**URL Bar Click:**
```
1. Set urlBoxFocused = true
2. Enable text input mode (SDL_StartTextInput)
3. Calculate cursor position based on mouse X
4. Allow editing
```

**Viewport Click:**
```
1. Set urlBoxFocused = false
2. Disable text input (SDL_StopTextInput)
3. Hit-test DOM tree
4. Find intersecting node
5. Trigger click handler for that node (if registered in Lua)
```

## State Management Per Tab

### Tab Creation Flow

```
1. Ctrl+T pressed
   ↓
2. TabManager::createTab() 
   ├─ Allocate new Tab object
   ├─ Assign unique ID
   └─ Initialize isolated Lua state
   ↓
3. loadTabContent(tab, "assets/index.jsml")
   ├─ Load file/URL
   ├─ Parse JSML → DOM tree
   ├─ Set as tab's domRoot
   ├─ Initialize Lua API for tab
   └─ Run embedded scripts
   ↓
4. Tab becomes active
   │ URL bar updates
   │ Viewport re-renders
   └─ Ready for interaction
```

### Tab Navigation Flow

```
1. URL entered and Enter pressed
   ↓
2. navigateTab(activeTab, url)
   ├─ Load file/URL
   ├─ Parse to new DOM tree
   ├─ Replace old DOM with new
   ├─ Re-initialize Lua environment
   └─ Run embedded scripts
   ↓
3. Main loop re-layouts and renders
   ├─ Layout: compute positions
   ├─ Render: prepare display
   └─ Present: swap SDL buffers
```

### Tab Switching Flow

```
1. Ctrl+Tab pressed
   ↓
2. TabManager::switchTab(nextTabId)
   ├─ Set activeTabId = nextTabId
   └─ Return new active tab
   ↓
3. Update main loop state
   ├─ URL bar = new tab's URL
   ├─ Viewport re-renders new tab's DOM
   └─ Input handling applies to new tab
```

### Tab Closing Flow

```
1. Ctrl+W pressed with Tab A active
   ↓
2. TabManager::closeTab(tabA.getId())
   ├─ Find Tab A in list
   ├─ Destroy Tab A (Lua shutdown, DOM cleanup)
   ├─ Remove from list
   └─ Set activeTab to next available
   ↓
3. Main loop switches to new active tab
   ├─ URL bar updates
   ├─ Viewport renders new tab
   └─ Continue rendering
```

## Advanced Features

### Lua API Per Tab

Each tab's Lua environment has access to:

```lua
-- Tab-specific functions (only affect current tab's DOM)
browser.log(msg)
browser.getCurrentUrl()
browser.getElemRect(id)
browser.fetch(url, callback)
browser.on(nodeId, "click", function() ... end)
```

Example:
```lua
-- Tab A's script
local myVar = "only in Tab A"
browser.on(5, "click", function()
    print(myVar)  -- Works fine
end)

-- Tab B never sees myVar
```

### TripleBuffer for Rendering

The render system uses a TripleBuffer for thread-safe rendering:

```
Logic Thread          Main/Render Thread
│                     │
├─ Write back buffer  │
│  (RenderCommands)   │
│                     ├─ Read front buffer
│                     │  (consistent snapshot)
├─ Call swapBuffers() │
│  (back → mid)       │
│                     └─ Front buffer updated
│                        on next frame
```

This allows future multi-threaded design where:
- Logic thread: DOM manipulation, Lua execution
- Render thread: Display only

## Troubleshooting

### Tab Won't Create

**Problem:** Ctrl+T doesn't create a new tab

**Solutions:**
1. Check max tabs limit (currently 32)
2. Verify assets/index.jsml exists
3. Check console for error messages

### URL Navigation Fails

**Problem:** Pressing Enter in URL bar doesn't navigate

**Solutions:**
1. Ensure URL bar is focused (click on it)
2. Check file/URL exists
3. Look for error messages in console

### Viewport Doesn't Update

**Problem:** Switching tabs doesn't show new content

**Solutions:**
1. Verify tab was created successfully
2. Check that content loaded into tab's DOM
3. Verify layout calculations (layoutNode called)

### Lua Scripts Not Working

**Problem:** Click handlers don't trigger

**Solutions:**
1. Verify browser.on() called and registered
2. Check node IDs are correct
3. Look at Lua error messages in console
4. Test with simpler handler first

## Performance Considerations

### Memory Usage

- Each tab: ~1-10MB (DOM tree + Lua state)
- 32 tabs max: ~320MB maximum
- Tab bar rendering: O(n) where n = number of tabs

### Rendering Performance

- Tab bar: Simple rects (~microseconds)
- URL bar: Text rendering (~milliseconds)
- Viewport: DOM rendering (varies with complexity)
- Target: 60 FPS (16ms/frame)

### Optimization Tips

1. Close unused tabs to free memory
2. Keep Lua scripts efficient
3. Avoid deeply nested DOM structures
4. Use local files (faster than network)

## Future Enhancements

Potential improvements:

1. **Tab bar click switching** - Click a tab to switch
2. **Tab close buttons** - X button on each tab
3. **Tab history** - Back/forward navigation per tab
4. **Pinned tabs** - Lock tabs to prevent accidental close
5. **Tab search** - Find tabs by URL
6. **Session save/restore** - Save/load tab states
7. **Shared resources** - Cache across tabs
8. **Async networking** - Non-blocking URL loads
9. **Worker threads** - Parallel Lua execution

## Summary

The tabbed interface provides:

✅ **True isolation** - each tab has own DOM, Lua, events  
✅ **Easy navigation** - Ctrl+T/W/Tab shortcuts  
✅ **Visual feedback** - tab bar shows state  
✅ **Full control** - URLs, files, network URLs supported  
✅ **Thread-safe rendering** - TripleBuffer architecture  

This creates a proper minimalist browser experience! 🎉
