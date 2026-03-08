local entityStates = Game.Entity.States

Game.World.OnWorldJoin:Connect(Async.create(function ()
    while Game.World.Loaded do
        if Game.LocalPlayer.getState(entityStates.ElytraFly) then
            Core.Debug.message("ElytraFly")
        end
        Async.wait()
    end
end))