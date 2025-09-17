local Gamepad = Game.Gamepad
local Debug = Core.Debug
local targetItem = nil

--- Backwards compatibility with 0.12.0
local OnJoinWorld = Game.World.OnWorldJoin or Game.Event.OnPlayerJoinWorld
local OnKeyPressed = Game.Gamepad.OnKeyPressed or Game.Event.OnKeyPressed

OnJoinWorld:Connect(function ()
    targetItem = Game.Items.findItemByName("diamond")
end)

-- Checks if player is holding a diamond
OnKeyPressed:Connect(function ()
    if Gamepad.isPressed(Gamepad.KeyCodes.DPADDOWN) then
        if Game.World.Loaded then
            local playerHand = Game.LocalPlayer.Inventory.Slots["hand"]
            if playerHand.Item == targetItem then  -- You can do this in different ways, like using playerHand.Item.NameID or Item.ID
                Debug.message("Player is holding a diamond")
            end
        end
    end
end)