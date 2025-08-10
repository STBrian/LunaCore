---@diagnostic disable: cast-local-type
Core.Debug.log("CoreAPI: Loading Items", false)
CoreAPI.Items = {}
---@type uvs_packer_funcs
local uvs_packer = dofile(Core.getModpath("LunaCoreAPI") .. "/src/uvs_packer.lua")
---@type uvs_builder_functions
local uvs_rebuilder = dofile(Core.getModpath("LunaCoreAPI") .. "/src/uvs_rebuilder.lua")
---@type atlas_handler_functions
local atlas_handler = dofile(Core.getModpath("LunaCoreAPI") .. "/src/atlas_handler.lua")
---@type blang_parser_funcs
local blang_parser = dofile(Core.getModpath("LunaCoreAPI") .. "/src/blang_parser.lua")

local function containsInvalidChars(s)
    if string.find(s, "[^%w_]") then
        return true
    else
        return false
    end
end

local itemsGlobals = {
    initialized = false,
    registry = {},
    allowedUVs = {},
    allowedAtlas = {},
    allowedTextures = {}
}

---@class ItemRegistry
local itemRegistry = CoreAPI.Utils.Classic:extend()

---Creates and returns an ItemRegistry object that allows to register custom items
---@param modname string
---@return ItemRegistry
function CoreAPI.Items.newItemRegistry(modname)
    if type(modname) ~= "string" then
        error("'modname' must be a string")
    end
    if containsInvalidChars(modname) then
        error("'modname' contains invalid characters")
    end
    return itemRegistry(modname)
end

function itemRegistry:new(modname)
    self.modname = modname
    self.definitions = {}
end

local function initItemRegistry()
    local titleId = Core.getTitleId()
    local basePath = string.format("sdmc:/luma/titles/%s/romfs", titleId)

    --- Warning about missing files
    --- Atlas UVs
    for packName, value in pairs(CoreAPI.ResourcePacks) do
        if not Core.Filesystem.fileExists(string.format("%s/atlas/atlas.items.meta_%08X.uvs", basePath, value.hash)) then
            Core.Debug.log(string.format("[Warning] CoreAPI: No atlas uvs found! Custom items won't have texture for '%s' pack. Please provide an atlas uvs under %s/atlas/atlas.items.meta_%08X.uvs", packName, basePath, value.hash), false)
        else
            table.insert(itemsGlobals.allowedUVs, packName)
        end
    end

    --- Atlas textures
    for packName, value in pairs(CoreAPI.ResourcePacks) do
        if not Core.Filesystem.fileExists(string.format("%s/atlas/atlas.items.meta_%08X_0.3dst", basePath, value.hash)) then
            Core.Debug.log(string.format("[Warning] CoreAPI: No atlas texture found! Custom items may not have texture for '%s' pack. Please provide an atlas texture under %s/atlas/atlas.items.meta_%08X_0.3dst", packName, basePath, value.hash), false)
        else
            table.insert(itemsGlobals.allowedTextures, packName)
        end
    end

    --- Locales
    for _, localeName in pairs(CoreAPI.Languages) do
        if not Core.Filesystem.fileExists(string.format("%s/loc/%s-pocket.blang", basePath, localeName)) then
            Core.Debug.log(string.format("[Warning] CoreAPI: No locale file found! Custom items won't have a locale name for '%s'. Please provide a locale file under %s/loc/%s-pocket.blang", localeName, basePath, localeName), false)
        end
    end
end

---Registers an item and sets other properties including its texture
---@param nameId string
---@param definition table
function itemRegistry:registerItem(nameId, definition)
    if type(nameId) ~= "string" then
        error("'nameId' must be a string")
    end
    if containsInvalidChars(nameId) then
        error("'nameId' contains invalid characters")
    end

    if not itemsGlobals.initialized then
        initItemRegistry()
        itemsGlobals.initialized = true
    end

    -- Validate item definition
    local itemDefinition = {}
    itemDefinition.name = string.lower(self.modname .. "_" .. nameId)
    itemDefinition.nameId = string.lower(self.modname .. ":" .. nameId)
    itemDefinition.locales = {}
    if itemsGlobals.registry[itemDefinition.nameId] ~= nil then
        error("Item '" .. itemDefinition.nameId "' is already registered")
    end
    itemDefinition.itemId = 254
    if type(definition.group) == "table" and definition.group.is ~= nil and definition.group:is(CoreAPI.ItemGroups.ItemGroupIdentifier) then
        itemDefinition.group = definition.group
    elseif type(definition.group) == "number" then
        itemDefinition.group = CoreAPI.ItemGroups.newItemGroupIdentifier(definition.group)
    end
    if type(definition) == "table" then
        if type(definition.locales) == "table" then
            for localeName, value in pairs(definition.locales) do
                if not CoreAPI.Utils.Table.contains(CoreAPI.Languages, localeName) then
                    error("Invalid locale '" .. localeName .. "'")
                else
                    if type(value) ~= "string" then
                        error("Invalid type for locale '" .. localeName .. "'")
                    else
                        itemDefinition.locales[localeName] = value
                    end
                end
            end
        end
        if type(definition.texture) == "string" then
            local modPath = Core.getModpath(self.modname)
            local texture = definition.texture
            if texture:match("^/") then
                texture = string.sub(texture, 2)
            end
            texture = string.gsub(texture, "\\", "/")
            local fullPath = string.format("%s/assets/textures/%s", modPath, texture)
            itemDefinition.texturePath = fullPath
            itemDefinition.texture = "textures/" .. texture:gsub(".3dst$", "")
        end
    end
    table.insert(self.definitions, itemDefinition)
    itemsGlobals.registry[itemDefinition.nameId] = {itemId = itemDefinition.itemId + 256, name = itemDefinition.name, locales = itemDefinition.locales}
end

function itemRegistry:registerItems()
    local titleId = Core.getTitleId()
    local basePath = string.format("sdmc:/luma/titles/%s/romfs", titleId)
    for packName, value in pairs(CoreAPI.ResourcePacks) do
        local uvsData = nil
        local uvsFile = Core.Filesystem.open(string.format("%s/atlas/atlas.items.meta_%08X.uvs", basePath, value.hash), "r+")
        if uvsFile == nil then
            Core.Debug.log(string.format("[Warning] CoreAPI: Failed to open UVs file. Custom items may not have texture for '%s' pack", packName), false)
        else
            uvsData = uvs_rebuilder.loadFile(uvsFile)
            if uvsData == nil then
                Core.Debug.log(string.format("[Warning] CoreAPI: Failed to parse UVs file. Custom items may not have texture for '%s' pack", packName), false)
            else
                local packer = uvs_packer.newPacker(uvsData, 16)
                for _, definition in ipairs(self.definitions) do
                    if not packer:addUV(definition.name, definition.texture) then
                        Core.Debug.log("[Warning] CoreAPI: Failed to register UV for item '" .. definition.nameId .. "'", false)
                    end
                end
                uvs_rebuilder.dumpFile(uvsFile, uvsData)
            end
            uvsFile:close()
        end
        if uvsData then
            local atlasFile = Core.Filesystem.open(string.format("%s/atlas/atlas.items.meta_%08X_0.3dst", basePath, value.hash), "r+")
            if not atlasFile then
                Core.Debug.log(string.format("[Warning] CoreAPI: Failed to open atlas file. Custom items may not have texture for '%s' pack", packName), false)
            else
                local handler = atlas_handler.newAtlasHandler(atlasFile)
                if not handler.parsed then
                    Core.Debug.log(string.format("[Warning] CoreAPI: Failed to parse atlas file. Custom items may not have texture for '%s' pack", packName), false)
                else
                    for _, definition in ipairs(self.definitions) do
                        local uvItem = uvsData[definition.name]
                        if uvItem ~= nil then
                            local errorCode = 1
                            local success = false
                            local texLoadFile = Core.Filesystem.open(definition.texturePath, "r")
                            if texLoadFile then
                                errorCode = 2
                                if handler:pasteTexture(texLoadFile, uvItem[1]["uv"][1], uvItem[1]["uv"][2]) then
                                    success = true
                                end
                                texLoadFile:close()
                            end
                            if not success then
                                Core.Debug.log("[Warning] CoreAPI: Failed to load texture '" .. definition.texturePath .. "'. Error: " .. errorCode, false)
                            end
                        end
                    end
                end
                atlasFile:close()
            end
        end
    end

    Game.Event.OnGameItemsRegister:Connect(function ()
        for _, definition in ipairs(self.definitions) do
            local regItem = Game.Items.registerItem(definition.name, definition.itemId)
            if regItem ~= nil then
                definition.item = regItem
                itemsGlobals.registry[definition.nameId].item = regItem
            else
                Core.Debug.log("[Warning] CoreAPI: Failed to register item '" .. definition.nameId .. "'", false)
            end
        end
    end)
    Game.Event.OnGameItemsRegisterTexture:Connect(function ()
        for _, definition in ipairs(self.definitions) do
            if definition.item ~= nil then
                definition.item:setTexture(definition.name, 0)
            end
        end
    end)
    Game.Event.OnGameCreativeItemsRegister:Connect(function ()
        for _, definition in ipairs(self.definitions) do
            if definition.group ~= nil and definition.group:is(CoreAPI.ItemGroups.ItemGroupIdentifier) and definition.item ~= nil then
                Game.Items.registerCreativeItem(definition.item, definition.group.id, definition.group:getCreativePosition())
            end
        end
    end)
end

--- Modify every locale file
for _, localeName in pairs(CoreAPI.Languages) do
    Game.Event.OnGameItemsRegister:Connect(function ()
        local count = 0
        for _ in pairs(itemsGlobals.registry) do
            count = count + 1
        end
        if count < 1 then
            return
        end
        local titleId = Core.getTitleId()
        local basePath = string.format("sdmc:/luma/titles/%s/romfs", titleId)
        if Core.Filesystem.fileExists(string.format("%s/loc/%s-pocket.blang", basePath, localeName)) then
            local localeFile = Core.Filesystem.open(string.format("%s/loc/%s-pocket.blang", basePath, localeName), "r+")
            if not localeFile then
                Core.Debug.log(string.format("[Warning] CoreAPI: Failed to open locale file. Custom items may not have names for '%s'", localeName), false)
            else
                local localeParser = blang_parser.newParser(localeFile)
                if not localeParser.parsed then
                    Core.Debug.log(string.format("[Warning] CoreAPI: Failed to parse locale file. Custom items may not have names for '%s'", localeName), false)
                else
                    local changed = false
                    for _, definition in pairs(itemsGlobals.registry) do
                        local itemName = definition.locales[localeName] or definition.locales["en_US"]
                        if itemName ~= nil then
                            if not localeParser:containsText("item." .. definition.name .. ".name") then
                                localeParser:addText("item." .. definition.name .. ".name", itemName)
                                changed = true
                            end
                        end
                    end
                    if changed then
                        localeParser:dumpFile(localeFile)
                    end
                end
                localeFile:close()
            end
        end
    end)
end

Game.Event.OnGameItemsRegister:Connect(function ()
    Core.Debug.log("[Info] CoreAPI: Register items", false)
    uvs_packer = nil
    uvs_rebuilder = nil
    atlas_handler = nil
end)
Game.Event.OnGameItemsRegisterTexture:Connect(function ()
    Core.Debug.log("[Info] CoreAPI: Register items texture", false)
end)
Game.Event.OnGameCreativeItemsRegister:Connect(function ()
    Core.Debug.log("[Info] CoreAPI: Register creative items", false)
end)

--- Get the item id with the item identifier
---@param itemName string
---@return integer?
function CoreAPI.Items.getItemId(itemName)
    local itemId
    itemName = string.lower(itemName)
    if string.match(itemName, "^minecraft:") then
        itemName = string.gsub(itemName, "^minecraft:", "")
        local item = Game.Items.findItemByName(itemName)
        if item then
            itemId = item.ID
        else
            itemId = nil
        end
    elseif not string.find(itemName, ":", 1, true) then
        local item = Game.Items.findItemByName(itemName)
        if item then
            itemId = item.ID
        else
            itemId = nil
        end
    else
        local instance = itemsGlobals.registry[itemName]
        if instance then
            itemId = instance.itemId
        else
            itemId = nil
        end
    end
    return itemId
end