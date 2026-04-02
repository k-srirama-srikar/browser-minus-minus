#### Lua is a simple programming language, famous for being embeddable in other programs.

#### This completely removes the problem of trying to write a full language runtime.

#### You might have to learn a little bit of Lua. Thankfully, it has only 21 keywords (compared

#### to ~40 in python).

# Requirements

#### Implement the browser table with the following functions. This will be executed by the

#### Lua script to interact with the webpage.

# TL;DR

```
-- DOM Manipulation
local elementIds = browser.getElemsByTag("tag-name")
local element = browser.getElem(elementIds[ 2 ]) -- returns JSON
element["spacing"] = 2 -- does not do anything
browser.updateElem(elementsId[ 2 ], element) -- updates the UI
```
```
browser.addClickHandler(elementIds[ 2 ], function()
click_handler(elementIds[ 2 ])
end)
```
```
local count = 0
function click_handler(id)
local element = getElem(id)
element["text"]["content"] = "clicked " .. count .. " time(s)"
browser.updateElem(id, element)
```
```
-- Basic telemetry
local unixTime = browser.getTime() -- in ms
local url = browser.getCurrentUrl()
```
```
-- Network
browser.fetch("/someAPI", function(response)
local element = getElem(elementIds[ 1 ])
element["text"]["content"] = response["content"]
browser.updateElem(elementIds[ 1 ])
end)
```

# Lua Browser Runtime API Documentation

#### This document outlines the Lua-based scripting interface for the toy browser engine. This

#### runtime is designed for high-performance benchmarking, replacing traditional JavaScript

#### with an embedded Lua environment.

## 1. Core Philosophy: The Update Cycle

#### Unlike a standard browser where the DOM is live and mutable, this engine uses a

#### Snapshot and Commit model. To change the UI, you must retrieve a representation of

#### an element, modify the Lua table, and "commit" it back to the engine.

## Key Concepts

## 2. DOM Manipulation

## getElemsByTag(tagName)

#### Returns a table (array) of IDs for all elements matching the tag.

## getElem(id)

#### Retrieves the state of a specific element as a Lua table.

```
-- Renderring
local rect = browser.getElemRect(elementIds[ 1 ]) -- array of x y w h
local screenSize = browser.getScreenSize() -- returns w h
```
```
-- Logging
browser.log("Hello!")
```
#### IDs: Elements are referenced by unique integer IDs.

#### 1-Based Indexing: All arrays (including those returned by the engine) follow standard

#### Lua 1-indexing.

#### Immutability: Changes to a table returned by getElem do nothing until updateElem

#### is called.

#### Returns: { id1, id2, ... idN }


### updateElem(id, table)

#### Commits changes from a Lua table back to the rendering engine. Use this to update

#### content or structure. This should be blocking.

## Creating New Elements

#### To add a new element, you modify the children array of an existing parent element.

## 3. Event Handling

### addClickHandler(id, callback)

#### Registers a Lua function to be executed when the element is clicked.

## 4. Networking & Telemetry

### fetch(url, callback)

#### Returns: A table containing properties like text, spacing, and bgcolor.

```
local parent = getElem(parentID)
-- Append a new element definition to the children table
table.insert(parent["flexv"], {
text = { content = "New Item" },
tag = "div",
spacing = 5
})
```
```
updateElem(parentID, parent)
```
#### id: The integer ID of the element.

#### callback: A function. It is recommended to pass an anonymous function or a

#### reference.

```
addClickHandler(buttonId, function()
log("Button was clicked!")
end)
```

#### Performs an asynchronous GET request. The response is automatically parsed from

#### JSON into a Lua table before being passed to the callback.

#### Argument Type Description

#### url String The endpoint to hit.

#### callback Function Receives the parsed Lua table as its only argument.

### getTime()

#### Returns the current Unix timestamp in milliseconds. Essential for benchmarking

#### execution time and network latency.

### getCurrentUrl()

#### Returns the string representation of the currently loaded page URL.

## 5. Layout & System Metrics

#### These functions are critical for benchmarks that measure the engine's layout performance

#### and screen utilisation.

### getElemRect(id)

#### Returns the calculated geometric bounds of an element after the last render pass.

### getScreenSize()

#### Returns the dimensions of the browser view-port.

## 7. Logging

### log(message)

#### Returns: { x, y, width, height }

**Returns:** (^) { width, height }


#### Outputs a string to the system console. Use this for debugging and reporting benchmark

#### results. On Fedora, this will typically pipe to stdout or your engine's debug panel.

#### Example Usage:

```
local start = getTime()
fetch("/api/data", function(data)
local end_time = getTime()
log("Network roundtrip: " .. (end_time - start) .. "ms")
end)
```

