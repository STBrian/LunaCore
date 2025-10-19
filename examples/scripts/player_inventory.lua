local Gamepad = Game.Gamepad
local localPlayer = Game.LocalPlayer
local Debug = Core.Debug
local World = Game.World

-- Tells what item is holding the player in hand
Game.Gamepad.OnKeyPressed:Connect(function ()
    if Gamepad.isDown(Gamepad.KeyCodes.DPADDOWN) and World.Loaded then -- World.Loaded ensure that the slot won't be nil
        local handSlot = localPlayer.Inventory.Slots["hand"]
        if not handSlot:isEmpty() then -- handSlot will be nil when player isn't loaded
            Debug.message("Player is holding item: " .. handSlot.Item.NameID)
        else
            Debug.message("Player's hand is empty")
        end
    end
end)

-- Tells what item is in slot 1 of the inventory
Game.Gamepad.OnKeyPressed:Connect(function ()
    if Gamepad.isDown(Gamepad.KeyCodes.DPADDOWN) and World.Loaded then -- World.Loaded ensure that the slot won't be nil
        local slot = localPlayer.Inventory.Slots[1] -- Slots 1 to 36
        if not slot:isEmpty() then
            Debug.message("Player has item in slot 1: " .. slot.Item.NameID)
        else
            Debug.message("Slot 1 is empty")
        end
    end
end)

-- Tells how many items are in slot 1
Game.Gamepad.OnKeyPressed:Connect(function ()
    if Gamepad.isDown(Gamepad.KeyCodes.DPADDOWN) and World.Loaded then -- World.Loaded ensure that the slot won't be nil
        local slot = localPlayer.Inventory.Slots[1] -- Slots 1 to 36
        if not slot:isEmpty() then
            Debug.message("Player has in slot 1 " .. slot.ItemCount .. " items")
        else
            Debug.message("Slot 1 is empty")
        end
    end
end)

-- Tells the variation of the item in slot 1
Game.Gamepad.OnKeyPressed:Connect(function ()
    if Gamepad.isDown(Gamepad.KeyCodes.DPADDOWN) and World.Loaded then -- World.Loaded ensure that the slot won't be nil
        local slot = localPlayer.Inventory.Slots[1] -- Slots 1 to 36
        if not slot:isEmpty() then
            Debug.message("Player has in slot 1 " .. slot.ItemData .. " item variation")
        else
            Debug.message("Slot 1 is empty")
        end
    end
end)