local gamepad = Game.Gamepad
local keys = gamepad.KeyCodes

local function drawGraphics(screen)
    if screen == "top" then
        Core.Graphics.drawRectFill(10, 10, 100, 100, Core.Graphics.colorRGB(0, 0, 0))
        Core.Graphics.drawText("Test text", 15, 15, Core.Graphics.colorRGB(255, 255, 255));
    end
end

-- You need to call open function to be able to draw on screen
-- The game will frezee and that allows to avoid artifacts on screen
-- Every other events and async tasks will continue execution
Game.Event.OnKeyReleased:Connect(function ()
    if gamepad.isReleased(keys.START) then
        if not Core.Graphics.isOpen() then
            Core.Graphics.open(drawGraphics)
        else
            Core.Graphics.close()
        end
    end
end)