# `Browser--`

A lightweight, embeddable browser engine written in modern C++23 with built-in Lua scripting support.


---

## Table of Contents

- [Overview](#overview)
- [Features & Capabilities](#features--capabilities)
- [JSML: A Custom Markup Format](#jsml-a-custom-markup-format)
  - [JSML Syntax](#jsml-syntax)
  - [Element Types](#element-types)
  - [Styling & Properties](#styling--properties)
  - [Complete Examples](#complete-examples)
- [Lua Runtime & Scripting](#lua-runtime--scripting)
  - [Browser API Reference](#browser-api-reference)
  - [Lua Integration](#lua-integration)
  - [Asynchronous Operations](#asynchronous-operations)
- [Web Capabilities](#web-capabilities)
- [Architecture & Implementation Details](#architecture--implementation-details)
- [Prerequisites & Dependencies](#prerequisites--dependencies)
- [Installation & Building](#installation--building)
  - [Quick Install](#quick-install)
  - [Manual Build](#manual-build)
  - [Build Options](#build-options)
- [Running the Browser](#running-the-browser)
  - [Basic Usage](#basic-usage)
  - [Loading JSML Documents](#loading-jsml-documents)
  - [Command-Line Arguments](#command-line-arguments)
- [Advanced Features](#advanced-features)
  - [Tabbed Browsing](#tabbed-browsing)
  - [Tab-Isolated Lua Environments](#tab-isolated-lua-environments)
  - [Dynamic Content Updates](#dynamic-content-updates)
  - [Scrolling & Viewport Management](#scrolling--viewport-management)
- [Examples](#examples)
- [Technical Notes](#technical-notes)

---

## Overview

**Browser++** is a minimalist browser engine designed for embedding and scripting. It combines a JSON-based markup language (JSML) with an integrated Lua 5.4 runtime to enable dynamic, interactive content creation without reliance on HTML, CSS, or JavaScript.

The engine features:
- **Purpose-built markup format**: JSON-based JSML eliminates parsing overhead and enables programmatic DOM manipulation
- **Integrated Lua 5.4 runtime**: Full Lua environment with tab-scoped execution contexts
- **Flex layout system**: Vertical and horizontal flex containers with responsive sizing
- **Network support**: HTTP/HTTPS resource fetching with libcurl
- **Modern C++23 implementation**: Built with contemporary C++ standards and practices
- **Multi-tab architecture**: Independent tabs with isolated DOM trees and Lua environments
- **Native rendering**: SDL3-based hardware-accelerated rendering with TTF text support
- **Image support**: Built-in image loading and rendering via SDL3_image

---

## Features & Capabilities

### Display Rendering
- **Text rendering**: Font size, color, and alignment configuration
- **Background colors**: Per-element background styling
- **Image rendering**: Embedded images from URLs or local files with caching
- **Layout engine**: Flexbox-inspired vertical and horizontal layouts
- **Scrolling support**: Viewport-based scrolling with scroll indicators
- **Tab bar**: Visual tab switching interface with dynamic tab creation and management
- **URL bar**: Interactive address bar for navigation

### Scripting & Interactivity
- **Click handling**: Element-level click events with Lua callbacks
- **DOM querying**: Find elements by ID or tag
- **DOM manipulation**: Runtime property updates
- **Element recreation**: Dynamic flexbox children modification
- **Event-driven architecture**: Fully reactive DOM updates

### Network Operations
- **HTTP/HTTPS fetching**: libcurl integration for resource loading
- **JSON parsing**: Automatic JSON to Lua conversion
- **File loading**: Local file access
- **URL resolution**: Relative path resolution against base URLs
- **Response caching**: Image texture caching

### Browser Operations
- **Multi-tab support**: Up to 32 concurrent tabs
- **Tab lifecycle**: Create, switch, and close tabs
- **Tab isolation**: Each tab has independent DOM, Lua environment, and scroll state
- **URL management**: Per-tab URL tracking and base URL resolution
- **History state**: Basic URL state management

---

## JSML: A Custom Markup Format

### JSML Syntax

JSML (JavaScript Object Markup Language) uses JSON as its foundation. A valid JSML document is a JSON object containing optional metadata and a root element:

```json
{
  "lua": "optional_script_path.lua",
  "root": { /* Element definition */ }
}
```

### Element Types

JSML supports five distinct element types:

#### 1. **FlexV (Vertical Container)**
Arranges children in a vertical stack (top to bottom).

```json
{
  "flexv": [
    { "text": { "content": "Item 1" } },
    { "text": { "content": "Item 2" } }
  ],
  "tag": "container",
  "spacing": 10
}
```

#### 2. **FlexH (Horizontal Container)**
Arranges children in a horizontal row (left to right).

```json
{
  "flexh": [
    { "text": { "content": "Left" } },
    { "text": { "content": "Right" } }
  ],
  "spacing": 15
}
```

#### 3. **Text Element**
Renders text content with configurable font and color.

```json
{
  "text": {
    "content": "Hello, World!",
    "fontsize": 24,
    "color": "#ffffff"
  },
  "tag": "greeting",
  "bgcolor": "#333333"
}
```

#### 4. **Image Element**
Displays images from URLs or local paths.

```json
{
  "image": {
    "url": "https://example.com/image.png",
    "alttext": "Description"
  },
  "width": 300,
  "height": 200,
  "tag": "photo"
}
```

#### 5. **Root Element**
Special container that wraps the entire document.

```json
{
  "root": {
    "flexv": [ /* Content */ ]
  }
}
```

### Styling & Properties

All elements support a set of extensible properties stored as a JSON property bag:

#### Text Properties
- **content** (string): Text to display
- **fontsize** (number): Font size in points (default: 16)
- **color** (string): Text color in hex format (default: "#000000")

#### Layout Properties
- **spacing** (number): Gap between children in flex containers (default: 8)
- **padding** (number): Internal padding around content (default: 12)
- **width** (number): Element width in pixels
- **height** (number): Element height in pixels

#### Visual Properties
- **bgcolor** (string): Background color in hex format
- **alttext** (string): Alternative text for images

#### Metadata
- **tag** (string or number): Unique identifier for element selection and event handling
  - Can be a string: `"tag": "myElement"`
  - Or a number: `"tag": 1`
  - Used for Lua DOM queries and click handlers

### Complete Examples

#### Simple Text Page
```json
{
  "root": {
    "flexv": [
      {
        "text": {
          "content": "Welcome",
          "fontsize": 32,
          "color": "#0066cc"
        },
        "tag": "title",
        "spacing": 20
      },
      {
        "text": {
          "content": "This is a simple text document.",
          "fontsize": 14,
          "color": "#666666"
        },
        "tag": "body"
      }
    ]
  }
}
```

#### Interactive Counter (with Lua)
```json
{
  "lua": "counter.lua",
  "root": {
    "flexv": [
      {
        "text": {
          "content": "0",
          "fontsize": 36,
          "color": "#000000"
        },
        "tag": 1,
        "spacing": 10
      },
      {
        "flexh": [
          {
            "text": {
              "content": "- Decrement",
              "fontsize": 16,
              "color": "#cc0000"
            },
            "tag": 2,
            "spacing": 5
          },
          {
            "text": {
              "content": "+ Increment",
              "fontsize": 16,
              "color": "#007700"
            },
            "tag": 3,
            "spacing": 5
          }
        ],
        "tag": 4,
        "spacing": 10
      }
    ]
  }
}
```

#### Image Gallery
```json
{
  "lua": "gallery.lua",
  "root": {
    "flexv": [
      {
        "text": {
          "content": "Image Gallery",
          "fontsize": 28,
          "color": "#ffffff"
        },
        "bgcolor": "#2c3e50",
        "spacing": 15
      },
      {
        "image": {
          "url": "https://http.cat/200",
          "alttext": "Cute cat image"
        },
        "width": 400,
        "height": 300,
        "tag": "img1",
        "spacing": 10
      }
    ]
  }
}
```

#### Complex Layout with Multiple Sections
```json
{
  "lua": "app.lua",
  "root": {
    "flexv": [
      {
        "text": {
          "content": "Dashboard",
          "fontsize": 24,
          "color": "#ffffff"
        },
        "bgcolor": "#1a1a2e",
        "spacing": 20,
        "tag": "header"
      },
      {
        "flexh": [
          {
            "flexv": [
              {
                "text": {
                  "content": "Section A",
                  "fontsize": 18,
                  "color": "#0066cc"
                },
                "tag": "sectionA",
                "spacing": 10
              },
              {
                "text": {
                  "content": "Content for section A",
                  "fontsize": 12,
                  "color": "#333333"
                }
              }
            ],
            "tag": "col1",
            "spacing": 10
          },
          {
            "flexv": [
              {
                "text": {
                  "content": "Section B",
                  "fontsize": 18,
                  "color": "#00aa00"
                },
                "tag": "sectionB",
                "spacing": 10
              },
              {
                "text": {
                  "content": "Content for section B",
                  "fontsize": 12,
                  "color": "#333333"
                }
              }
            ],
            "tag": "col2",
            "spacing": 10
          }
        ],
        "tag": "mainContent",
        "spacing": 15
      }
    ]
  }
}
```

---

## Lua Runtime & Scripting

### Browser API Reference

Browser++ exposes a comprehensive Lua API through the `browser` global table. The following functions are available within Lua scripts:

#### Query & Traversal

**`browser.getElem(id: number) -> table | nil`**
- Retrieves a complete element object by its numeric ID
- Returns a table with properties: `id`, `tag`, and all element properties
- Returns `nil` if element not found
- Example:
  ```lua
  local elem = browser.getElem(42)
  if elem then
    print("Element tag:", elem.tag)
  end
  ```

**`browser.getElemsByTag(tag: string | number) -> table`**
- Queries all elements matching a given tag
- Returns a Lua table (array) of element IDs
- Supports both string and numeric tags
- Example:
  ```lua
  local buttons = browser.getElemsByTag("button")
  for i, id in ipairs(buttons) do
    print("Button ID:", id)
  end
  ```

**`browser.getElemRect(id: number) -> table | nil`**
- Returns the computed layout rectangle for an element
- Returns table: `{x, y, width, height}` in pixels
- Example:
  ```lua
  local rect = browser.getElemRect(10)
  if rect then
    print(string.format("Position: (%d, %d), Size: %dx%d", 
      rect[1], rect[2], rect[3], rect[4]))
  end
  ```

#### DOM Manipulation

**`browser.updateElem(id: number, properties: table) -> void`**
- Updates element properties in-place
- Properties are stored as JSON values
- Supports updating `flexv` and `flexh` children arrays
- Example:
  ```lua
  local elem = browser.getElem(15)
  elem["text"]["content"] = "Updated text"
  elem["bgcolor"] = "#ff0000"
  browser.updateElem(15, elem)
  ```

**Creating dynamic content:**
  ```lua
  local parent = browser.getElem(1)
  parent["flexv"] = {
    {tag = "child1", text = {content = "New child 1"}},
    {tag = "child2", text = {content = "New child 2"}}
  }
  browser.updateElem(1, parent)
  ```

#### Event Handling

**`browser.addClickHandler(id: number, callback: function) -> void`**
- Registers a click event handler for an element
- Handler is called when the user clicks the element
- Example:
  ```lua
  browser.addClickHandler(42, function()
    print("Element 42 was clicked!")
    browser.log("Counting clicks...")
  end)
  ```

**Click counter example:**
  ```lua
  local count = 0
  browser.addClickHandler(10, function()
    count = count + 1
    local elem = browser.getElem(10)
    elem["text"]["content"] = "Clicked " .. count .. " times"
    browser.updateElem(10, elem)
  end)
  ```

#### Network Operations

**`browser.fetch(url: string, callback: function([response] | nil)) -> void`**
- Performs asynchronous HTTP/HTTPS fetch or file load
- URL can be absolute HTTP/HTTPS, or relative path
- Callback receives parsed JSON or raw response
- Example:
  ```lua
  browser.fetch("data.json", function(response)
    if response then
      print("Fetched data:", response)
    else
      print("Fetch failed")
    end
  end)
  ```

**Remote Lua script loading:**
  ```lua
  browser.fetch("https://api.example.com/script.lua", function(script)
    if script then
      local chunk = load(script)
      if chunk then chunk() end
    end
  end)
  ```

#### Utility Functions

**`browser.log(message: string) -> void`**
- Prints a message to console (stdout)
- Useful for debugging
- Prefixed with `[Lua Log]` in output
- Example:
  ```lua
  browser.log("Script initialized successfully")
  ```

**`browser.getTime() -> number`**
- Returns current Unix timestamp in milliseconds
- Example:
  ```lua
  local startTime = browser.getTime()
  -- ... do work ...
  local elapsed = browser.getTime() - startTime
  print("Elapsed:", elapsed, "ms")
  ```

**`browser.getScreenSize() -> table`**
- Returns viewport dimensions
- Returns table: `{width, height}` in pixels
- Example:
  ```lua
  local screen = browser.getScreenSize()
  print("Viewport:", screen[1] .. "x" .. screen[2])
  ```

**`browser.getCurrentUrl() -> string`**
- Returns the current tab's URL
- Example:
  ```lua
  local url = browser.getCurrentUrl()
  print("Currently viewing:", url)
  ```

### Lua Integration

#### Tab-Scoped Execution
Each tab maintains its own isolated Lua 5.4 runtime. This enables:
- Independent script execution per tab
- Isolated global state and variables
- Separate click handlers and event contexts
- No cross-tab variable pollution

#### Script Loading
JSML documents specify their Lua script via the `"lua"` property:

```json
{
  "lua": "path/to/script.lua",
  "root": { /* ... */ }
}
```

Scripts can be:
- Relative paths: `"script.lua"` (resolved relative to JSML document)
- Absolute HTTP/HTTPS URLs: `"https://example.com/app.lua"`
- Local filesystem paths: Can use `/` for absolute paths

#### Luagen Standard Libraries
The runtime includes access to these Lua 5.4 standard libraries:
- `base`: Core Lua functions (`print`, `type`, `ipairs`, etc.)
- `package`: Module loading (`require`, `dofile`)
- `string`: String manipulation (`string.upper`, `string.sub`, etc.)
- `math`: Mathematical functions
- `table`: Table utilities (`table.insert`, `table.concat`, etc.)
- `os`: Operating system interface

### Asynchronous Operations

**Fetch Handling**
The `browser.fetch()` function is non-blocking:
- Returns immediately after dispatch
- Callback will be invoked when resource load completes
- Supports both success and failure cases

**Example: Chained Async Operations**
```lua
browser.fetch("config.json", function(config)
  if config and config.dataUrl then
    browser.fetch(config.dataUrl, function(data)
      if data then
        -- Process fetched data
      end
    end)
  end
end)
```

---

## Web Capabilities

### Network Features
- **HTTP/HTTPS support**: Full URL fetching via libcurl
- **Follow redirects**: Automatic HTTP redirect handling
- **Resource caching**: Images cached by URL to reduce memory usage
- **Local file access**: Can load resources from filesystem
- **URL resolution**: Automatic relative→absolute URL conversion

### Data Format Support
- **JSON parsing**: Automatic JSON→Lua table conversion
- **Raw text**: Non-JSON responses treated as strings
- **Binary data**: Image loading and caching

### Resource Types
- **Images**: PNG, JPEG, BMP, and more (via SDL3_image)
- **Code**: Lua scripts fetched from remote URLs
- **Data**: JSON, plain text, and raw binary

---

## Architecture & Implementation Details

### Core Components

#### 1. **DOM (Document Object Model)**
- Implemented in `dom.hpp/cpp`
- Tree-based structure with node types: `Root`, `FlexV`, `FlexH`, `Text`, `Image`
- JSON property bags for extensibility
- Unique numeric IDs for all nodes
- Supports recursive traversal and querying

#### 2. **Layout Engine**
- File: `layout.cpp`
- Flexbox-inspired vertical and horizontal layouts
- Text measurement and wrapping logic
- Dynamic height calculation
- Viewport-aware sizing

#### 3. **Renderer**
- File: `renderer.cpp`
- SDL3 hardware-accelerated rendering
- TTF font support with per-size caching
- Image texture caching
- Tab bar and URL bar rendering
- Scrollbar visualization

#### 4. **Network Module**
- File: `network.cpp`
- libcurl HTTP/HTTPS fetching
- Local file loading
- Callback-based response handling

#### 5. **Scripting Engine**
- File: `scripting.cpp`
- Sol2 Lua bindings
- Tab-scoped Lua environments
- Browser API implementation
- JSON↔Lua conversion utilities

#### 6. **Tab Management**
- File: `tab.cpp`
- Multi-tab support (up to 32 tabs)
- Per-tab DOM trees
- Isolated Lua environments
- Tab-specific scroll and URL state

#### 7. **Thread-Safe Rendering System**
- File: `buffer.hpp`
- Triple-buffer pattern for concurrent rendering
- Decouples logic thread from render thread
- **RenderCommand**: Individual rendering instruction (text, rect, image, clear)
- **TripleBuffer<T>**: Template class managing three buffers in rotation
  - Back buffer: Logic thread writes render commands
  - Mid buffer: Intermediate staging area
  - Front buffer: Render thread reads from here
- Mutex-protected buffer swapping for thread safety
- Zero-copy buffer passing between threads

### Data Flow

1. **Initialization**: Main creates TabManager and loads initial content
2. **Parsing**: JSML fetched and parsed into DOM tree
3. **Layout**: DOM tree measured and laid out to viewport
4. **Rendering**: Layout tree rendered via SDL3
5. **Scripting**: Lua runtime initialized and attached to tab
6. **Events**: User clicks trigger element lookup and Lua callbacks
7. **Updates**: Lua can fetch data and modify DOM programmatically

### Thread-Safe Rendering Architecture

The rendering system uses a triple-buffer pattern to safely pass data between the logic thread (updating DOM) and the render thread (drawing to SDL3):

#### Buffer States and Flow

```
Time T₀:                   Time T₁:                   Time T₂:
┌──────────────┐          ┌──────────────┐          ┌──────────────┐
│ Back:  Empty │  <──     │ Back: Empty  │  <──     │ Back: Empty  │
│ Mid:   Ready │  ──>     │ Mid: Drawing │  ──>     │ Mid: Drawing │
│ Front: Draw  │          │ Front: Ready │          │ Front: Draw  │
└──────────────┘          └──────────────┘          └──────────────┘
Logic writes  Lock swap   Render reads  Lock swap   Render reads
to Back       Mid←→Back   from Front    Mid←→Back   from Front
```

#### RenderCommand Structure

Each rendering instruction is encapsulated in a `RenderCommand` struct containing:

| Field | Type | Purpose |
|-------|------|---------|
| `type` | enum | Instruction type: `DrawText`, `DrawRect`, `DrawImage`, `Clear` |
| `x`, `y` | float | Pixel coordinates for rendering |
| `width`, `height` | float | Dimensions in pixels |
| `content` | string | Text content (for DrawText) |
| `color` | string | Hex color value (e.g., "#ff0000") |
| `imagePath` | string | URL/path to image (for DrawImage) |
| `fontSize` | int | Font size in points |

#### TripleBuffer Implementation

The `TripleBuffer<T>` template class manages concurrent access to three buffers:

**Key Methods:**

- `getBackBuffer()`: Returns reference to write buffer (logic thread safe)
- `getBackRenderBuffer()`: Convenience accessor used by Tab class
- `swapBuffers()`: Atomically rotates mid↔back; called after logic updates
- `getFrontBuffer()`: Returns front buffer for reading; swaps mid↔front if available (render thread safe)
- `clearAll()`: Clears all three buffers

**Thread Safety:**
- Back buffer: No cross-thread access (logic thread exclusive)
- Mid buffer: Protected by `updateMutex` during swaps
- Front buffer: Protected by `readMutex` during swaps
- Never requires both threads to wait simultaneously

#### Usage in Tab Class

Each `Tab` instance contains `TripleBuffer<RenderCommand> renderTripleBuffer` with these methods:

```cpp
// Build render commands (logic thread)
std::vector<RenderCommand>& commands = getBackRenderBuffer();
commands.push_back(RenderCommand(...));  // No locking needed

// After updates complete, rotate buffers
swapRenderBuffers();

// In render loop, consume the front buffer
const std::vector<RenderCommand>& toRender = getFrontRenderBuffer();
// Render each command to SDL3 texture
```

#### Advantages of Triple-Buffering

1. **No blocking**: Logic thread never waits for render thread
2. **Consistent snapshots**: Render thread always sees complete, consistent frames
3. **Smooth updates**: New render data available every frame
4. **Lock-free efficiency**: Minimal mutex contention via buffer rotation

---

## Prerequisites & Dependencies

### System Requirements
- **OS**: Linux, macOS, or Windows
- **Compiler**: C++23 support (GCC 13+, Clang 16+, or MSVC 2022+)
- **CMake**: Version 3.20 or newer
- **Display**: X11, Wayland, or compatible display server (or headless-compatible)

### Core Dependencies

| Library | Version | Purpose |
|---------|---------|---------|
| SDL3 | 3.2.0+ | Window management, rendering, event handling |
| SDL3_ttf | 3.2.0+ | TrueType font rendering |
| SDL3_image | 3.2.0+ | Image format loading (PNG, JPEG, BMP, etc.) |
| libcurl | 7.0+ | HTTP/HTTPS network requests |
| Lua | 5.4.x | Scripting runtime |
| Nlohmann JSON | 3.11+ | JSON parsing and manipulation |
| Sol2 | 3.3+ | Lua C++ bindings |

### Installation

#### Debian / Ubuntu
```bash
sudo apt update
sudo apt install -y \
  build-essential \
  cmake \
  pkg-config \
  libcurl4-openssl-dev \
  libsdl3-dev \
  libsdl3-ttf-dev \
  libsdl3-image-dev \
  liblua5.4-dev
```

#### Fedora / RHEL / CentOS
```bash
sudo dnf install -y \
  gcc-c++ \
  cmake \
  pkgconf-pkg-config \
  libcurl-devel \
  SDL3-devel \
  SDL3_ttf-devel \
  SDL3_image-devel \
  lua-devel
```

#### macOS (Homebrew)
```bash
brew install cmake curl lua sdl3 sdl3_ttf sdl3_image
```

#### Windows (MSVC)
Use the vcpkg package manager or download prebuilt dependencies.

---

## Installation & Building

### Quick Install

For users on Debian/Ubuntu-based systems, use the provided installation script:

```bash
chmod +x install.sh
./install.sh
```

This script will:
1. Detect your OS and package manager
2. Install all required dependencies
3. Create the build directory
4. Compile the project
5. Place the executable in `build/browser`

### Manual Build

#### Step 1: Clone and Navigate
```bash
git clone https://github.com/k-srirama-srikar/browser-minus-minus
cd browser++
```

#### Step 2: Create Build Directory
```bash
mkdir -p build
cd build
```

#### Step 3: Configure with CMake
```bash
cmake ..
```

For release optimization:
```bash
cmake -DCMAKE_BUILD_TYPE=Release ..
```

#### Step 4: Compile
```bash
cmake --build . -j$(nproc)
```

For verbose output (useful for debugging):
```bash
cmake --build . --verbose
```

#### Step 5: Verify Build
The executable will be at `build/browser`:
```bash
ls -la ./browser
```

### Build Options

#### Debug Build
```bash
cmake -DCMAKE_BUILD_TYPE=Debug ..
cmake --build .
```
Produces debug symbols for use with GDB/LLDB.

#### Release Build (Optimized)
```bash
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build .
```
Produces optimized binary with no debug symbols.

#### Custom Compiler
```bash
cmake -DCMAKE_CXX_COMPILER=clang++ ..
```

#### Verbose Output
```bash
cmake --build . -- VERBOSE=1
```

#### Parallel Build
```bash
cmake --build . -j 4
```

---

## Running the Browser

### Basic Usage

After a successful build, launch the browser:

```bash
./build/browser
```

This opens an empty browser window with:
- Tab bar at the top
- URL input bar
- Viewport area
- Scrollbar (if content exceeds viewport)

### Loading JSML Documents

#### From Command Line
Provide a JSML file path as an argument:

```bash
./build/browser assets/index.jsml
```

#### From URL
Load a remote JSML document by providing an HTTP/HTTPS URL:

```bash
./build/browser https://example.com/app.jsml
```

#### From File System
Absolute or relative paths work:

```bash
./build/browser ./assets/tests/test1.jsml
./build/browser /home/user/documents/page.jsml
```

### Command-Line Arguments

| Argument | Effect |
|----------|--------|
| `<path/url>` | Load initial JSML document |
| (none) | Open with blank first tab |

### GUI Interactions

#### Keyboard Shortcuts
- **Ctrl+T**: Create new tab
- **Ctrl+W**: Close active tab
- **Ctrl+Tab**: Cycle to next tab
- Standard text editing in URL bar

#### Mouse Interactions
- Click tab bar to switch tabs
- Click `x` button on tabs to close
- Click URL bar to edit address
- Scroll wheel in viewport to scroll content
- Click elements to trigger click handlers

---

## Advanced Features

### Tabbed Browsing

Browser++ supports up to 32 concurrent tabs. Each tab is completely independent:

```lua
-- Get current tab's URL
local url = browser.getCurrentUrl()

-- Fetch data into current tab
browser.fetch("data.json", function(data)
  print("Tab URL:", url)
end)
```

#### Tab Management via GUI
- **New tab**: Ctrl+T or click + button in tab bar
- **Switch tab**: Click desired tab name
- **Close tab**: Click × button on tab
- **Previous tab**: Click left arrow (if tabs overflow)
- **Next tab**: Click right arrow (if tabs overflow)

### Tab-Isolated Lua Environments

Each tab maintains a completely separate Lua runtime:

```
Tab 1 (app.jsml)         Tab 2 (game.jsml)        Tab 3 (data.jsml)
├─ DOM tree              ├─ DOM tree               ├─ DOM tree
├─ Lua state             ├─ Lua state              ├─ Lua state
├─ Click handlers        ├─ Click handlers         ├─ Click handlers
├─ Global variables      ├─ Global variables       ├─ Global variables
└─ Scroll state          └─ Scroll state           └─ Scroll state
```

This means:
- Scripts in Tab 1 cannot access Tab 2's Lua variables
- Click handlers are scoped to their tab
- Each tab can load different Lua libraries without conflicts

### Dynamic Content Updates

JSML documents can modify their DOM at runtime via Lua:

```lua
-- Update text content
local elem = browser.getElem(1)
elem["text"]["content"] = os.date("%Y-%m-%d %H:%M:%S")
browser.updateElem(1, elem)

-- Modify children
local container = browser.getElem(5)
container["flexv"] = {
  {tag = 1, text = {content = "Item 1"}},
  {tag = 2, text = {content = "Item 2"}},
  {tag = 3, text = {content = "Item 3"}}
}
browser.updateElem(5, container)
```

### Scrolling & Viewport Management

The browser supports content scrolling when content exceeds the viewport:

#### Scrolling Methods
1. **Mouse wheel**: Scroll up/down in viewport
2. **Keyboard**: (Partial support)
3. **Lua API**: (Future expansion)

#### Viewport Dimensions
Query viewport size from Lua:

```lua
local screen = browser.getScreenSize()
local width = screen[1]
local height = screen[2]
print(string.format("Viewport: %dx%d", width, height))
```

---

## Examples

### Example 1: Simple Text Display

**File: `simple.jsml`**
```json
{
  "root": {
    "flexv": [
      {
        "text": {
          "content": "Hello, Browser++!",
          "fontsize": 28,
          "color": "#ffffff"
        },
        "bgcolor": "#2c3e50",
        "spacing": 20
      },
      {
        "text": {
          "content": "This is a simple document.",
          "fontsize": 14,
          "color": "#34495e"
        }
      }
    ]
  }
}
```

Run:
```bash
./build/browser simple.jsml
```

### Example 2: Interactive Counter Application

**File: `counter.jsml`**
```json
{
  "lua": "counter.lua",
  "root": {
    "flexv": [
      {
        "text": {
          "content": "Counter App",
          "fontsize": 24,
          "color": "#ffffff"
        },
        "bgcolor": "#1a1a2e",
        "spacing": 20
      },
      {
        "text": {
          "content": "0",
          "fontsize": 64,
          "color": "#0066cc"
        },
        "tag": 1,
        "spacing": 10
      },
      {
        "flexh": [
          {
            "text": {
              "content": "➖ Decrement",
              "fontsize": 16,
              "color": "#ffffff"
            },
            "tag": 2,
            "bgcolor": "#cc0000",
            "spacing": 5
          },
          {
            "text": {
              "content": "➕ Increment",
              "fontsize": 16,
              "color": "#ffffff"
            },
            "tag": 3,
            "bgcolor": "#00aa00",
            "spacing": 5
          }
        ],
        "spacing": 10
      }
    ]
  }
}
```

**File: `counter.lua`**
```lua
local displayId = browser.getElemsByTag(1)[1]
local decId = browser.getElemsByTag(2)[1]
local incId = browser.getElemsByTag(3)[1]

local count = 0

local function updateDisplay()
  local elem = browser.getElem(displayId)
  elem["text"]["content"] = tostring(count)
  browser.updateElem(displayId, elem)
end

browser.addClickHandler(incId, function()
  count = count + 1
  updateDisplay()
  browser.log("Count: " .. count)
end)

browser.addClickHandler(decId, function()
  count = count - 1
  updateDisplay()
  browser.log("Count: " .. count)
end)

browser.log("Counter app initialized")
```

### Example 3: Data Fetching

**File: `weather.jsml`**
```json
{
  "lua": "weather.lua",
  "root": {
    "flexv": [
      {
        "text": {
          "content": "Weather App",
          "fontsize": 24,
          "color": "#ffffff"
        },
        "bgcolor": "#3498db",
        "spacing": 20
      },
      {
        "text": {
          "content": "Loading...",
          "fontsize": 16,
          "color": "#333333"
        },
        "tag": "weather",
        "spacing": 10
      }
    ]
  }
}
```

**File: `weather.lua`**
```lua
browser.fetch("https://api.example.com/weather.json", function(data)
  if data then
    local weatherElem = browser.getElem(browser.getElemsByTag("weather")[1])
    weatherElem["text"]["content"] = "Temperature: " .. data.temp .. "°C"
    browser.updateElem(browser.getElemsByTag("weather")[1], weatherElem)
  else
    local weatherElem = browser.getElem(browser.getElemsByTag("weather")[1])
    weatherElem["text"]["content"] = "Failed to load weather"
    browser.updateElem(browser.getElemsByTag("weather")[1], weatherElem)
  end
end)
```

### Example 4: Multi-Section Dashboard

**File: `dashboard.jsml`**
```json
{
  "lua": "dashboard.lua",
  "root": {
    "flexv": [
      {
        "text": {
          "content": "Dashboard",
          "fontsize": 28,
          "color": "#ffffff"
        },
        "bgcolor": "#2c3e50",
        "spacing": 20,
        "tag": "header"
      },
      {
        "flexh": [
          {
            "flexv": [
              {
                "text": {
                  "content": "Section A",
                  "fontsize": 18,
                  "color": "#0066cc"
                },
                "tag": "sectionA",
                "spacing": 10
              },
              {
                "text": {
                  "content": "Content A",
                  "fontsize": 12,
                  "color": "#666666"
                },
                "tag": "contentA"
              }
            ],
            "tag": "colA",
            "spacing": 10
          },
          {
            "flexv": [
              {
                "text": {
                  "content": "Section B",
                  "fontsize": 18,
                  "color": "#00aa00"
                },
                "tag": "sectionB",
                "spacing": 10
              },
              {
                "text": {
                  "content": "Content B",
                  "fontsize": 12,
                  "color": "#666666"
                },
                "tag": "contentB"
              }
            ],
            "tag": "colB",
            "spacing": 10
          }
        ],
        "spacing": 15
      }
    ]
  }
}
```

---

## Technical Notes

### JSON Parsing Considerations

JSML uses strict JSON parsing with one enhancement:

**Trailing Comma Tolerance**
The parser automatically strips trailing commas before parsing:

```json
{
  "root": {
    "flexv": [
      { "text": { "content": "Item 1" } },
      { "text": { "content": "Item 2" } },  // Trailing comma allowed
    ]
  }
}
```

This relaxation aids development but is not standard JSON.

### Font Rendering

The renderer searches for TrueType fonts in this order:

1. Environment variable `BROWSER_FONT`
2. Environment variable `FONT`
3. Local asset paths (`src/assets/`, `assets/`)
4. System font directories:
   - Linux: `/usr/share/fonts/truetype/`
   - macOS: `/Library/Fonts/`, `/System/Library/Fonts/`
   - Windows: `C:\Windows\Fonts\`

If no font is found, text rendering is disabled.

### Image Caching

Images are cached by URL to reduce memory usage:

- First load fetches and renders the image
- Subsequent references to same URL use cached texture
- Cache is cleared when the browser exits

### URL Resolution

Relative URLs are resolved against the base URL:

| Base URL | Relative | Result |
|----------|----------|--------|
| `http://example.com/app/` | `data.json` | `http://example.com/app/data.json` |
| `http://example.com/app/index.jsml` | `lib/lib.lua` | `http://example.com/app/lib/lib.lua` |
| `http://example.com/app/` | `/assets/img.png` | `http://example.com/assets/img.png` |
| `/home/user/docs/` | `file.lua` | `/home/user/docs/file.lua` |

### Performance Considerations

- **Layout**: O(n) tree traversal per render frame
- **Text rendering**: Cached font objects per size
- **Images**: Cached textures by URL
- **Lua**: Efficient Sol2 bindings with minimal overhead
- **Memory**: DOM tree size is proportional to element count

### Known Limitations

- Maximum 32 tabs per browser instance
- No persistent tab history
- No cookies or session storage
- Lua environment reset on page reload
- Single-threaded architecture
- No CSS support (styling via properties instead)
- Text wrapping based on character estimates (not pixel-perfect)
