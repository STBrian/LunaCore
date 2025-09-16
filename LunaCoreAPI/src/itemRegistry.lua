---@class ItemRegistry
local itemRegistry = CoreAPI.Utils.Classic:extend()

local function containsInvalidChars(s)
    if string.find(s, "[^%w_]") then return true else return false end
end

---@type uvs_packer_funcs
local uvs_packer = dofile(Core.getModpath("LunaCoreAPI") .. "/src/uvs_packer.lua")
---@type uvs_builder_functions
local uvs_rebuilder = dofile(Core.getModpath("LunaCoreAPI") .. "/src/uvs_rebuilder.lua")
---@type atlas_handler_functions
local atlas_handler = dofile(Core.getModpath("LunaCoreAPI") .. "/src/atlas_handler.lua")
---@type blang_parser_funcs
local blang_parser = dofile(Core.getModpath("LunaCoreAPI") .. "/src/blang_parser.lua")

local OnGameRegisterCreativeItems = Game.Event.OnGameCreativeItemsRegister or Game.Event.OnGameRegisterCreativeItems
local OnGameRegisterItems = Game.Event.OnGameItemsRegister or Game.Event.OnGameRegisterItems
local OnGameRegisterItemsTextures = Game.Event.OnGameItemsRegisterTexture or Game.Event.OnGameRegisterItemsTextures

local Registry = CoreAPI.Items.Registry

local itemRegistryGlobals = {
    initialized = false,
    allowedUVs = {},
    allowedAtlas = {},
    allowedTextures = {}
}

---Init ItemRegistry globals
local function initItemRegistry()
    local titleId = Core.getTitleId()
    local basePath = string.format("sdmc:/luma/titles/%s/romfs", titleId)

    --- Warning about missing files
    --- Atlas UVs
    for packName, value in pairs(CoreAPI.ResourcePacks) do
        if not Core.Filesystem.fileExists(string.format("%s/atlas/atlas.items.meta_%08X.uvs", basePath, value.hash)) then
            Core.Debug.log(string.format("[Warning] CoreAPI: No atlas uvs found! Custom items won't have texture for '%s' pack. Please provide an atlas uvs under %s/atlas/atlas.items.meta_%08X.uvs", packName, basePath, value.hash), false)
        else
            table.insert(itemRegistryGlobals.allowedUVs, packName)
        end
    end

    --- Atlas textures
    for packName, value in pairs(CoreAPI.ResourcePacks) do
        if not Core.Filesystem.fileExists(string.format("%s/atlas/atlas.items.meta_%08X_0.3dst", basePath, value.hash)) then
            Core.Debug.log(string.format("[Warning] CoreAPI: No atlas texture found! Custom items may not have texture for '%s' pack. Please provide an atlas texture under %s/atlas/atlas.items.meta_%08X_0.3dst", packName, basePath, value.hash), false)
        else
            table.insert(itemRegistryGlobals.allowedTextures, packName)
        end
    end

    --- Locales
    for _, localeName in pairs(CoreAPI.Languages) do
        if not Core.Filesystem.fileExists(string.format("%s/loc/%s-pocket.blang", basePath, localeName)) then
            Core.Debug.log(string.format("[Warning] CoreAPI: No locale file found! Custom items won't have a locale name for '%s'. Please provide a locale file under %s/loc/%s-pocket.blang", localeName, basePath, localeName), false)
        end
    end

    itemRegistryGlobals.initialized = true
end

---comment
---@param modname string
function itemRegistry:new(modname)
    self.modname = modname
    self.definitions = {}
    self.registeredTextures = {}
end

---Registers an item and sets other properties including its texture
---@param nameId string
---@param definition table
function itemRegistry:registerItem(nameId, itemId, definition)
    if type(nameId) ~= "string" then
        error("'nameId' must be a string")
    end
    if containsInvalidChars(nameId) then
        error("'nameId' contains invalid characters")
    end

    if not itemRegistryGlobals.initialized then
        initItemRegistry()
    end

    -- Validate item definition
    local regNameId = string.lower(self.modname .. ":" .. nameId)
    if Registry[regNameId] ~= nil then
        error("Item '" .. regNameId "' is already registered")
    end
    local gameNameId = string.lower(self.modname .. "_" .. nameId)

    local itemDefinition = {}
    itemDefinition.name = gameNameId
    itemDefinition.nameId = regNameId
    itemDefinition.locales = {}

    -- Default values
    itemDefinition.group = CoreAPI.ItemGroups.Creative.newPositionIdentifier(CoreAPI.ItemGroups.FOOD_MINERALS, 0x7FFF)
    itemDefinition.stackSize = 64
    itemDefinition.hasTexture = false

    itemDefinition.itemId = itemId
    if type(definition) == "table" then
        if type(definition.group) == "table" then
            itemDefinition.group = CoreAPI.ItemGroups.Creative.newPositionIdentifier(definition.group[1], definition.group[2])
        elseif type(definition.group) == "number" then
            itemDefinition.group = CoreAPI.ItemGroups.Creative.newPositionIdentifier(definition.group, 0x7FFF)
        end

        if type(definition.locales) == "table" then
            for localeName, value in pairs(definition.locales) do
                if not CoreAPI.Utils.Table.contains(CoreAPI.Languages, localeName) then
                    error("Invalid locale '" .. localeName .. "'")
                else
                    if type(value) ~= "string" then
                        error("Expected string for locale text '" .. localeName .. "'")
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
            if not Core.Filesystem.fileExists(fullPath) then
                Core.Debug.log("[Warning] CoreAPI: Texture path '" .. fullPath .. "' doesn't exists", false)
            else
                itemDefinition.texturePath = fullPath
                itemDefinition.texture = "textures/" .. texture:gsub(".3dst$", "")
                itemDefinition.textureName = texture:gsub(".3dst$", "")
                itemDefinition.hasTexture = true
            end
        end

        if type(definition.stackSize) == "number" then
            if definition.stackSize < 1 then
                error("stackSize must be greater than 1")
            end
            itemDefinition.stackSize = definition.stackSize
        end
    end
    table.insert(self.definitions, itemDefinition)
    Registry[regNameId] = {itemId = itemId + 256, name = gameNameId, locales = itemDefinition.locales, item = nil}
end

---Returns true if any change was made
---@param packer UVs_packer
---@param definition any
---@return boolean
local function registerUV(packer, definition)
    if packer:contains(definition.textureName:gsub("/", "_")) then
        return false
    end
    if not packer:addUV(definition.textureName:gsub("/", "_"), definition.texture) then
        Core.Debug.log("[Warning] CoreAPI: Failed to register UV for item '" .. definition.nameId .. "'", false)
        return false
    end
    return true
end

--- It returns true if the function executes without errors. The UVs data is copied to out table
---@param pack table
---@param packName string
---@param out table?
---@return boolean
function itemRegistry:modifyPackUVs(pack, packName, out)
    local titleId = Core.getTitleId()
    local basePath = string.format("sdmc:/luma/titles/%s/romfs", titleId)

    local uvsFile = Core.Filesystem.open(string.format("%s/atlas/atlas.items.meta_%08X.uvs", basePath, pack.hash), "r+")
    if not uvsFile then
        Core.Debug.log(string.format("[Warning] CoreAPI: Failed to open UVs file. Custom items may not have texture for '%s' pack", packName), false)
        return false
    end

    local uvsData = uvs_rebuilder.loadFile(uvsFile)
    if not uvsData then
        Core.Debug.log(string.format("[Warning] CoreAPI: Failed to parse UVs file. Custom items may not have texture for '%s' pack", packName), false)
        return false
    end

    local packer = uvs_packer.newPacker(uvsData, 16)
    local changed = false
    for _, definition in ipairs(self.definitions) do
        if definition.hasTexture then
            changed = registerUV(packer, definition) or changed
        end
    end
    if changed then
        uvs_rebuilder.dumpFile(uvsFile, uvsData)
    end
    uvsFile:close()

    if type(out) == "table" then
        for key, value in pairs(uvsData) do
            out[key] = value
        end
    end

    return true
end

--- Returns if succeeded
---@param uvItem table
---@param definition table
---@param handler AtlasHandler
---@return boolean
local function pasteTextureToAtlas(uvItem, definition, handler)
    local texLoadFile = Core.Filesystem.open(definition.texturePath, "r")
    if not texLoadFile then
        Core.Debug.log("[Warning] CoreAPI: Failed to open texture '" .. definition.texturePath .. "'", false)
        return false
    end

    -- File is updated automatically to 
    if not handler:pasteTexture(texLoadFile, uvItem[1]["uv"][1], uvItem[1]["uv"][2]) then
        Core.Debug.log("[Warning] CoreAPI: Failed to load texture '" .. definition.texturePath .. "'", false)
        return false
    end

    texLoadFile:close()
    return true
end

function itemRegistry:modifyTextureAtlas(pack, packName, uvsData)
    local titleId = Core.getTitleId()
    local basePath = string.format("sdmc:/luma/titles/%s/romfs", titleId)

    local atlasFile = Core.Filesystem.open(string.format("%s/atlas/atlas.items.meta_%08X_0.3dst", basePath, pack.hash), "r+")
    if not atlasFile then
        Core.Debug.log(string.format("[Warning] CoreAPI: Failed to open atlas file. Custom items may not have texture for '%s' pack", packName), false)
        return false
    end

    local handler = atlas_handler.newAtlasHandler(atlasFile)
    if not handler.parsed then
        Core.Debug.log(string.format("[Warning] CoreAPI: Failed to parse atlas file. Custom items may not have texture for '%s' pack", packName), false)
        atlasFile:close()
        return false
    end

    for _, definition in ipairs(self.definitions) do
        if definition.hasTexture then
            local uvItem = uvsData[definition.textureName:gsub("/", "_")] or uvsData[CoreAPI.Utils.String.hash(definition.textureName:gsub("/", "_"))]
            if uvItem then
                if not CoreAPI.Utils.Table.contains(self.registeredTextures, definition.textureName) then
                    pasteTextureToAtlas(uvItem, definition, handler)
                    table.insert(self.registeredTextures, definition.textureName)
                end
            end
        end
    end

    atlasFile:close()
    return true
end

function itemRegistry:registerItems()
    for packName, value in pairs(CoreAPI.ResourcePacks) do
        local uvsData = {}
        if self:modifyPackUVs(value, packName, uvsData) then
            self:modifyTextureAtlas(value, packName, uvsData)
        end
    end

    OnGameRegisterItems:Connect(function ()
        for _, definition in ipairs(self.definitions) do
            local regItem = Game.Items.registerItem(definition.name, definition.itemId)
            if regItem ~= nil then
                definition.item = regItem
                Registry[definition.nameId].item = regItem
                regItem.StackSize = definition.stackSize
            else
                Core.Debug.log("[Warning] CoreAPI: Failed to register item '" .. definition.nameId .. "'", false)
            end
        end
    end)
    OnGameRegisterItemsTextures:Connect(function ()
        for _, definition in ipairs(self.definitions) do
            if definition.item ~= nil then
                if definition.hasTexture then
                    definition.item:setTexture(definition.textureName:gsub("/", "_"), 0)
                else
                    definition.item:setTexture("apple", 0)
                end
            end
        end
    end)
    OnGameRegisterCreativeItems:Connect(function ()
        for _, definition in ipairs(self.definitions) do
            if definition.item ~= nil and CoreAPI.Utils.isinstance(definition.group, CoreAPI.ItemGroups.Creative.GroupPositionIdentifier) then
                Game.Items.registerCreativeItem(definition.item, definition.group.id, definition.group:getCreativePosition())
            end
        end
    end)
end

--- Modify every locale file
for _, localeName in pairs(CoreAPI.Languages) do
    OnGameRegisterItems:Connect(function ()
        local count = 0
        for _ in pairs(Registry) do
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
                    for _, definition in pairs(Registry) do
                        local itemName = definition.locales[localeName] or definition.locales["en_US"]
                        if itemName ~= nil then
                            if not (localeParser:containsText("item."..definition.name..".name") and localeParser:areEqual("item."..definition.name..".name", itemName)) then
                                localeParser:addText("item." .. definition.name .. ".name", itemName)
                                changed = true
                            end
                        end
                    end
                    if changed then
                        localeParser:dumpFile(localeFile)
                        collectgarbage("collect")
                    end
                end
                localeFile:close()
            end
        end
        collectgarbage("collect")
    end)
end

return itemRegistry