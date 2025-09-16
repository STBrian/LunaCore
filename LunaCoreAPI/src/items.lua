---@diagnostic disable: cast-local-type

CoreAPI.Items = {}
CoreAPI.Items.Registry = {}

local function containsInvalidChars(s)
    if string.find(s, "[^%w_]") then return true else return false end
end

local OnGameRegisterCreativeItems = Game.Event.OnGameCreativeItemsRegister or Game.Event.OnGameRegisterCreativeItems
local OnGameRegisterItems = Game.Event.OnGameItemsRegister or Game.Event.OnGameRegisterItems
local OnGameRegisterItemsTextures = Game.Event.OnGameItemsRegisterTexture or Game.Event.OnGameRegisterItemsTextures

---@type ItemRegistry
local itemRegistry = dofile(Core.getModpath("LunaCoreAPI") .. "/src/itemRegistry.lua")

local itemsGlobals = {
    initializedItems = false
}

---Creates and returns an ItemRegistry object that allows to register custom items
---@param modname string
---@return ItemRegistry
function CoreAPI.Items.newItemRegistry(modname)
    if itemsGlobals.initializedItems then
        error("New items must be registered on mod load")
    end
    if type(modname) ~= "string" then
        error("'modname' must be a string")
    end
    if containsInvalidChars(modname) then
        error("'modname' contains invalid characters")
    end
    local modPath = Core.getModpath(modname)
    if modPath == nil then
        error("Modname not registered")
    end
    return itemRegistry(modname)
end

OnGameRegisterItems:Connect(function ()
    Core.Debug.log("[Info] CoreAPI: Register items", false)
    itemRegistry = nil -- Deletes functions that won't be necessary after this event
    itemsGlobals.initializedItems = true
end)
OnGameRegisterItemsTextures:Connect(function ()
    Core.Debug.log("[Info] CoreAPI: Register items texture", false)
end)
OnGameRegisterCreativeItems:Connect(function ()
    Core.Debug.log("[Info] CoreAPI: Register creative items", false)
end)

--- Get the item id with the item identifier
---@param itemName string
---@return integer?
function CoreAPI.Items.getItemId(itemName)
    local itemId = nil
    itemName = string.lower(itemName)
    if string.match(itemName, "^minecraft:") then
        itemName = string.gsub(itemName, "^minecraft:", "")
        local item = Game.Items.findItemByName(itemName)
        if item then itemId = item.ID end
    elseif not string.find(itemName, ":", 1, true) then
        local item = Game.Items.findItemByName(itemName)
        if item then itemId = item.ID end
    else
        local instance = CoreAPI.Items.Registry[itemName]
        if instance then itemId = instance.itemId end
    end
    return itemId
end