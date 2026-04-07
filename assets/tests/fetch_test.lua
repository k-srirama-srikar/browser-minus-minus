browser.log("LUA LOADED")
browser.fetch("fetch_test.json", function(data)
    if data and data.a == 1 then
        browser.log("FETCH SUCCESS: " .. tostring(data.a))
    else
        browser.log("FETCH FAILED")
    end
end)
