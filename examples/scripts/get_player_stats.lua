local player = Game.LocalPlayer
local debug = Core.Debug
local gamepad = Game.Gamepad

Game.Gamepad.OnKeyPressed:Connect(function ()
    if Game.World.Loaded then
        if gamepad.isPressed(gamepad.KeyCodes.DPADLEFT) then
            debug.message("Player current hp: "..player.CurrentHP)
            debug.message("Player current hunger: "..player.CurrentHunger)
            debug.message("Player current level: "..player.CurrentLevel)
        end
    end
end)