local player = Game.LocalPlayer
local gamepad = Game.Gamepad
local coalItem = nil
local diamondItem = nil
local first = true

Game.World.OnWorldJoin:Connect(function ()
    -- You cannot use findItem when the game isn't loaded, wait until World is loaded to prevent nil results
    coalItem = Game.Items.findItemByName("coal") -- Make sure to not call this function many times as it can lag the game, use one time and store the value
    diamondItem = Game.Items.findItemByName("diamond")
end)

Game.Gamepad.OnKeyPressed:Connect(function () -- You can connect functions when other events are triggered
    if Game.World.Loaded and coalItem and diamondItem then -- Make sure all is loaded
        if gamepad.isDown(gamepad.KeyCodes.DPADDOWN) then
            local playerHand = player.Inventory.Slots["hand"]
            if not playerHand:isEmpty() and playerHand.Item == coalItem and playerHand.ItemData == 0 then
                playerHand:setItem(diamondItem)
            end
        end
    end
end)