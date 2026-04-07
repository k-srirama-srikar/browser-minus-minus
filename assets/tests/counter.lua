local displayIds = browser.getElemsByTag(1)
local decIds     = browser.getElemsByTag(2)
local incIds     = browser.getElemsByTag(3)

local count = 0

local function refresh()
  local el = browser.getElem(displayIds[1])
  el["text"]["content"] = "Counter " .. count
  browser.updateElem(displayIds[1], el)
end

browser.addClickHandler(incIds[1], function()
  count = count + 1
  refresh()
end)

browser.addClickHandler(decIds[1], function()
  count = count - 1
  refresh()
end)
