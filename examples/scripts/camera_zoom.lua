local playerCamera = Game.LocalPlayer.Camera
local Gamepad = Game.Gamepad
local originalFOV = 70
local isZoom = false

Game.Gamepad.OnKeyPressed:Connect(function ()
    if Gamepad.isDown(Gamepad.KeyCodes.ZL) and Gamepad.isDown(Gamepad.KeyCodes.ZR) then
        if Game.World.Loaded then
            local playerHand = Game.LocalPlayer.Inventory.Slots["hand"]
            if playerHand.Item and playerHand.Item.NameID == "diamond" then
                if isZoom then
                    playerCamera.FOV = originalFOV
                    isZoom = false
                else
                    playerCamera.FOV = originalFOV - 30
                    isZoom = true
                end
            end
        end
    end
end)