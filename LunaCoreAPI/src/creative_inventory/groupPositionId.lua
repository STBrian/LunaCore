---@class CreativeGroupPositionID
local creativePositionIdentifier = CoreAPI.Utils.Classic:extend()

function creativePositionIdentifier:new(id, position)
    self.id = id
    if CoreAPI.Utils.isinstance(position, CoreAPI.ItemGroups.Creative.RelativePosition) then
        self.relativePosition = position
    elseif type(position) == "number" then
        self.position = position
    else
        self.position = 0x0
    end
end

function creativePositionIdentifier:getCreativePosition()
    if type(self.relativePosition) ~= "table" then
        return self.position
    end
    local itemId = self.relativePosition:getId()
    if itemId == nil then
        Core.Debug.log("[Warning] CoreAPI: Failed to get ID for '" .. self.relativePosition.id .. "'", false)
        return 0x7FFF
    end
    local position = Game.Items.getCreativePosition(itemId, self.id)
    if position == 0x7FFF then
        Core.Debug.log("[Warning] CoreAPI: Failed to get creative position for ID '" .. itemId .. "'", false)
    end
    if self.relativePosition.origin == 2 then
        position = position - 1
    end
    return position
end

return creativePositionIdentifier