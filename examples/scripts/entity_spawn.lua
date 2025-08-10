local Debug = Core.Debug

--Game.Event.OnGameEntitySpawn:Connect(function ()
Game.Event.OnGameEntitySpawn:Connect(function ()
    Debug.message("New entity spawned")
end)
