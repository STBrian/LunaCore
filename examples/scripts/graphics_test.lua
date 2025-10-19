local white = Core.Graphics.colorRGB(255, 255, 255)
local black = Core.Graphics.colorRGB(0, 0, 0)

local function drawTextLeft(text, x, y, color)
    local textWidth = Core.Graphics.getTextWidth(text)
    Core.Graphics.drawText(text, x - textWidth, y, color)
end

Core.Graphics.OnNewFrame:Connect(function (screen)
    if screen == "top" then
        drawTextLeft("Hunger: "..tostring(Game.LocalPlayer.CurrentHunger), 400 - 10, 75, black)
        drawTextLeft("FOV: "..tostring(Game.LocalPlayer.Camera.FOV), 400 - 10, 85, black)
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