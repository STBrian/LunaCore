local Gamepad = Game.Gamepad
local Debug = Core.Debug

--- Backwards compatibility with 0.12.0
local OnKeyPressed = Game.Gamepad.OnKeyPressed or Game.Event.OnKeyPressed

-- You need to call Slots["hand"] everytime you want to check hand slot as it wont 
-- update any reference you saved previously
OnKeyPressed:Connect(function ()
    if Gamepad.isPressed(Gamepad.KeyCodes.DPADDOWN) then
        if Game.World.Loaded then
            local playerHand = Game.LocalPlayer.Inventory.Slots["hand"]
            if not playerHand:isEmpty() then
                Debug.message("ItemID: "..playerHand.Item.ID)
                Debug.message("Item Name: "..playerHand.Item.NameID)
                Debug.message("Item Data: "..playerHand.ItemData)
                Debug.message("Item Count: "..playerHand.ItemCount)
            end
        end
    end
end)