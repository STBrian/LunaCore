local Debug = Core.Debug

Game.Event.OnGameEntitySpawnStart:Connect(function (eventObject)
    Debug.message("Entity x: " .. tostring(eventObject.x) .. " y: " .. tostring(eventObject.y) .. " z: " .. tostring(eventObject.z))

    -- return table with new coords
    return {
        x = eventObject.x + 10.0,
        y = 20.0,
        z = 30.0
    }
end)
