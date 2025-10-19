local memory = Core.Memory
local gamepad = Game.Gamepad
local keys = gamepad.KeyCodes
local debug = Core.Debug
local viewBobbingSensibilityOffset = 0x2CF2A0+0x100000

Game.Gamepad.OnKeyPressed:Connect(function ()
    if gamepad.isPressed(keys.Y) then
        local viewBobbingSensibility = memory.readFloat(viewBobbingSensibilityOffset)
        if viewBobbingSensibility ~= nil then
            debug.message("ViewBobbing Sensibility value: "..viewBobbingSensibility)
        end
    end

    if gamepad.isPressed(keys.X) then
        if memory.writeFloat(viewBobbingSensibilityOffset, 8) then
            debug.message("Success")
        else
            debug.message("Failed")
        end
    end
end)