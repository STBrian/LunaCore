local Gamepad = Game.Gamepad
local Debug = Core.Debug

-- You need to call Slots["hand"] everytime you want to check hand slot as it wont 
-- update any reference you saved previously
Game.Gamepad.OnKeyPressed:Connect(function ()
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