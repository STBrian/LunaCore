local isDown = Game.Gamepad.isDown
local keys = Game.Gamepad.KeyCodes

Game.Gamepad.OnKeyPressed:Connect(function ()
    if Game.World.Loaded then
        if isDown(keys.DPADLEFT) then
            Game.World.Weather = Enums.WeatherType.Clear
        elseif isDown(keys.DPADRIGHT) then
            Game.World.Weather = "Rain"
        elseif isDown(keys.DPADDOWN) then
            Game.World.Weather = Enums.WeatherType.Thunder
        end
    end
end)