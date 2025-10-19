local World = Game.World
local Debug = Core.Debug
local lastWorldLoaded = World.Loaded

-- Method 1
Game.World.OnWorldJoin:Connect(function ()
    Debug.message("Player joined a world")
end)

Game.World.OnWorldLeave:Connect(function ()
    Debug.message("Player leaved a world")
end)

-- Method 2
Async.create(function ()
    while Async.wait() do
        if lastWorldLoaded ~= World.Loaded then
            lastWorldLoaded = World.Loaded
            if lastWorldLoaded == true then
                Debug.message("Player joined a world")
            else
                Debug.message("Player leaved a world")
            end
        end
    end
end)