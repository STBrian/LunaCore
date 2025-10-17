local white = Core.Graphics.colorRGB(255, 255, 255)
local black = Core.Graphics.colorRGB(0, 0, 0)

local function drawTextLeft(text, x, y, color)
    local textWidth = Core.Graphics.getTextWidth(text)
    Core.Graphics.drawText(text, x - textWidth, y, color)
end

Core.Graphics.OnNewFrame:Connect(function (screen)
    if screen == "top" then
        drawTextLeft("WorldLoaded: "..tostring(Game.World.Loaded), 400 - 10, 5, black)
        drawTextLeft("Flying: "..tostring(Game.LocalPlayer.Flying), 400 - 10, 15, black)
        drawTextLeft("Sneaking: "..tostring(Game.LocalPlayer.Sneaking), 400 - 10, 25, black)
        drawTextLeft("Sprinting: "..tostring(Game.LocalPlayer.Sprinting), 400 - 10, 35, black)
        drawTextLeft("Jumping: "..tostring(Game.LocalPlayer.Jumping), 400 - 10, 45, black)
        drawTextLeft("OnGround: "..tostring(Game.LocalPlayer.OnGround), 400 - 10, 55, black)
        drawTextLeft("Dimension: "..tostring(Game.LocalPlayer.Dimension), 400 - 10, 65, black)
        drawTextLeft("Hunger: "..tostring(Game.LocalPlayer.CurrentHunger), 400 - 10, 75, black)
    end
end)

--[[ You need to call open function to be able to draw on screen
-- The game will frezee and that allows to avoid artifacts on screen
-- Every other events and async task will continue execution
Game.Event.OnKeyReleased:Connect(function ()
    if gamepad.isReleased(keys.START) then
        if not Core.Graphics.isOpen() then
            Core.Graphics.open(drawGraphics)
        else
            Core.Graphics.close()
        end
    end
end)]]