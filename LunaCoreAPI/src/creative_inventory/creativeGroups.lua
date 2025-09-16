local modPath = Core.getModpath("LunaCoreAPI")
---@type CreativeGroupPositionID
local groupPositionId = dofile(modPath .. "/src/creative_inventory/groupPositionId.lua")
---@type CreativeGroupRelativePosition
local relativeGroupPosition = dofile(modPath .. "/src/creative_inventory/relativePosition.lua")

CoreAPI.ItemGroups.Creative = {}
CoreAPI.ItemGroups.Creative.GroupPositionIdentifier = groupPositionId
CoreAPI.ItemGroups.Creative.RelativePosition = relativeGroupPosition

---Return an instance that is used as an identifier of a creative group position
---@param id number
---@param position (number|CreativeGroupRelativePosition)|?
---@return CreativeGroupPositionID
function CoreAPI.ItemGroups.Creative.newPositionIdentifier(id, position)
    if id < 1 or id > 7 then
        error("Invalid creative group ID")
    end
    if not CoreAPI.Utils.isinstance(position, CoreAPI.ItemGroups.Creative.RelativePosition) and type(position) ~= "number" and position ~= nil then
        error("Invalid position. Expected 'RelativePosition', 'number' or 'nil'")
    end
    return groupPositionId(id, position)
end

---Returns a table that can be used as relative position from an item id
---@param id number|string nameID or itemID
---@return CreativeGroupRelativePosition
function CoreAPI.ItemGroups.Creative.afterItem(id)
    if type(id) ~= "number" and type(id) ~= "string" then
        error("Invalid item name or item id")
    end
    if type(id) == "string" then
        id = id:lower()
    end
    return relativeGroupPosition(id, relativeGroupPosition.AFTER)
end

---Returns a table that can be used as relative position from an item id
---@param id number|string nameID or itemID
---@return CreativeGroupRelativePosition
function CoreAPI.ItemGroups.Creative.beforeItem(id)
    if type(id) ~= "number" and type(id) ~= "string" then
        error("Invalid item name or item id")
    end
    return relativeGroupPosition(id, relativeGroupPosition.BEFORE)
end