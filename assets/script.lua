browser.log("Lua Initialized!")

local counters = browser.getElemsByTag("counter")
if #counters == 0 then
    browser.log("Error: could not find generic 'counter' tag!")
else
    local cId = counters[1]
    local count = 0

    browser.addClickHandler(cId, function()
        count = count + 1
        browser.log("Button Clicked! Count: " .. count)

        local element = browser.getElem(cId)
        element["text"]["content"] = "Clicked " .. count .. " time(s)"
        browser.updateElem(cId, element)
    end)
    
    browser.log("Successfully bound click handler to element ID " .. cId)
end
