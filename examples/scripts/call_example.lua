-- Call registerItem function
local shortMemory = Core.Memory.malloc(2)
if shortMemory then
    Core.Memory.writeS16(shortMemory, 250)
    local res = Core.Memory.call(0x007cdc8c, "sP", "P", "call_example", shortMemory)
    Core.Debug.log("Call result: " .. res, true)
    local item = Game.Items.findItemByID(250 + 256)
    if item then
        Game.Items.OnRegisterCreativeItems:Connect(function ()
            Game.Items.registerCreativeItem(item, 1, -1)
        end)
        Core.Debug.message("Registered")
    end
    Core.Memory.free(shortMemory)
end