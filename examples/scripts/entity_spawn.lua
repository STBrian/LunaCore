local Debug = Core.Debug

Game.Event.OnGameEntitySpawnStart:Connect(function (event)
    Debug.message("Entity x: " .. tostring(event.x) .. " y: " .. tostring(event.y) .. " z: " .. tostring(event.z))

    -- Modify the spawn coordinates
    event.y = 20.0
end)
