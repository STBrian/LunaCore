local Gamepad = Game.Gamepad
local Debug = Core.Debug
local targetItem = nil

Game.World.OnWorldJoin:Connect(function ()
    targetItem = Game.Items.findItemByName("diamond")
end)

-- Checks if player is holding a diamond
Game.Gamepad.OnKeyPressed:Connect(function ()
    if Gamepad.isPressed(Gamepad.KeyCodes.DPADDOWN) then
        if Game.World.Loaded then
            local playerHand = Game.LocalPlayer.Inventory.Slots["hand"]
            if playerHand.Item == targetItem then  -- You can do this in different ways, like using playerHand.Item.NameID or Item.ID
                Debug.message("Player is holding a diamond")
            end
        end
    end
end)