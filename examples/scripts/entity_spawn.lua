local Debug = Core.Debug

Game.Event.OnGameEntitySpawnStart:Connect(function (target)
    Debug.message("Entity x: " .. tostring(target.X) .. " y: " .. tostring(target.Y) .. " z: " .. tostring(target.Z))

    -- Modify the spawn coordinates
    target.Y = target.Y + 20.0
end)
