---@class CreativeGroupRelativePosition
local relativeCreativePosition = CoreAPI.Utils.Classic:extend()

relativeCreativePosition.AFTER = 1
relativeCreativePosition.BEFORE = 2

function relativeCreativePosition:new(id, origin)
    self.id = id -- string or number
    self.origin = origin
end

---Returns the item id of the item origin
---@return integer?
function relativeCreativePosition:getId()
    if type(self.id) == "string" then
        local itemId = CoreAPI.Items.getItemId(self.id)
        return itemId
    else
        return self.id
    end
end

return relativeCreativePosition