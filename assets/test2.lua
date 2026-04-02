local clickCount = {
    ["button-one"] = 0,
    ["button-two"] = 0,
    ["button-three"] = 0
}

local function safeGetFirstId(tag)
    local ids = browser.getElemsByTag(tag)
    if ids and ids[1] then
        return ids[1]
    end
    return nil
end

local function updateElementText(id, message)
    if not id then
        return
    end
    local element = browser.getElem(id)
    if not element or not element["text"] then
        return
    end
    element["text"]["content"] = message
    browser.updateElem(id, element)
end

local function updateStatus(message)
    local statusId = safeGetFirstId("fetch-status")
    if statusId then
        local statusElem = browser.getElem(statusId)
        statusElem["text"]["content"] = message
        browser.updateElem(statusId, statusElem)
    end
end

local function refreshLayoutSummary()
    local urlId = safeGetFirstId("current-url")
    if urlId then
        local value = browser.getCurrentUrl() or "unknown"
        updateElementText(urlId, "Current URL: " .. value)
    end

    local screenId = safeGetFirstId("screen-size")
    if screenId then
        local screen = browser.getScreenSize()
        if screen then
            local width = screen[1] or 0
            local height = screen[2] or 0
            updateElementText(screenId, string.format("Screen size: %dx%d", width, height))
        end
    end
end

local function attachClickHandler(tag, func)
    local ids = browser.getElemsByTag(tag)
    if not ids then
        return
    end
    for i = 1, #ids do
        local id = ids[i]
        browser.addClickHandler(id, function()
            func(id)
        end)
    end
end

local function clickHandler(id, label)
    local now = browser.getTime()
    clickCount[label] = (clickCount[label] or 0) + 1
    local element = browser.getElem(id)
    if element and element["text"] then
        element["text"]["content"] = string.format("%s clicked %d time(s) at %d ms", label, clickCount[label], now)
        element["bgcolor"] = label == "button-one" and "#1d4ed8" or label == "button-two" and "#047857" or "#ca8a04"
        browser.updateElem(id, element)
    end
    updateStatus(string.format("Last click: %s (count=%d)", label, clickCount[label]))
end

local function hydrateFetchContent(response)
    local contentId = safeGetFirstId("fetched-content")
    if not contentId then
        return
    end
    local content = browser.getElem(contentId)
    local message = "No response"
    if response then
        if response["content"] then
            message = response["content"]
        elseif type(response) == "string" then
            message = response
        else
            message = "Fetched response received"
        end
    end
    content["text"]["content"] = message
    browser.updateElem(contentId, content)
end

local function showElementBounds(tag)
    local ids = browser.getElemsByTag(tag)
    if ids and ids[1] then
        local rect = browser.getElemRect(ids[1])
        if rect then
            browser.log(string.format("%s rect = [%.1f, %.1f, %.1f, %.1f]", tag, rect[1], rect[2], rect[3], rect[4]))
        end
    end
end

-- Initialize interactive page state
refreshLayoutSummary()
updateStatus("Ready to fetch and respond to clicks.")

attachClickHandler("button-one", function(id)
    clickHandler(id, "button-one")
end)
attachClickHandler("button-two", function(id)
    clickHandler(id, "button-two")
end)
attachClickHandler("button-three", function(id)
    clickHandler(id, "button-three")
end)
attachClickHandler("tab", function(id)
    clickHandler(id, "tab")
end)

showElementBounds("hero-image")

browser.fetch("assets/test2_data.json", function(response)
    if response then
        hydrateFetchContent(response)
        browser.log("Fetch completed and updated content.")
    else
        browser.log("Fetch failed or returned invalid response.")
        updateStatus("Fetch failed. Check assets/test2_data.json or fetch implementation.")
    end
end)
