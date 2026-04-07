local function benchmark()
    browser.log("Starting Benchmark...")

    local ids = browser.getElemsByTag("main-container")
    if #ids == 0 then
        browser.log("Error: main-container not found")
        return
    end
    local rootId = ids[1]
    local rootNode = browser.getElem(rootId)

    local startTime = browser.getTime()

    for i = 1, 100 do
        local box = {
            tag = 100 + i,
            bgcolor = (i % 2 == 0) and "#ff0000" or "#0000ff", -- alternating red/blue
            spacing = 2,
            text = {
                content = "Box #" .. i,
                fontsize = 12,
                color = "#ffffff"
            }
        }
        table.insert(rootNode["flexv"], box)
    end

    browser.updateElem(rootId, rootNode)

    local createdTime = browser.getTime()
    browser.log("Created 100 boxes in: " .. (createdTime - startTime) .. "ms")

    local header = rootNode["flexv"][1]
    rootNode["flexv"] = { header }

    browser.updateElem(rootId, rootNode)

    local deletedTime = browser.getTime()
    browser.log("Deleted 100 boxes in: " .. (deletedTime - createdTime) .. "ms")
    browser.log("Total cycle time: " .. (deletedTime - startTime) .. "ms")
end

benchmark()
