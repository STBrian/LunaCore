local white = Core.Graphics.colorRGB(255, 255, 255)
local black = Core.Graphics.colorRGB(0, 0, 0)

---@param obj GLabel
---@param text string
---@param x integer
---@param y integer
local function setAlignLeft(obj, text, x, y)
    local textWidth = Core.Graphics.getTextWidth(text)
    obj:setPosition(x - textWidth, y)
    obj:setText(text)
end

local objects = {
    hungerLabel = Core.Graphics.newLabel(0, 0, "Hunger: "),
    fovLabel = Core.Graphics.newLabel(0, 0, "FOV: ")
}

objects.hungerLabel:setSystemFont(true)
objects.hungerLabel:setColor(black)
objects.hungerLabel:setScreens(true, false)
objects.hungerLabel:setVisible(false)
objects.fovLabel:setSystemFont(true)
objects.fovLabel:setColor(black)
objects.fovLabel:setScreens(true, false)
objects.fovLabel:setVisible(false)
setAlignLeft(objects.hungerLabel, "Hunger: ", 400 - 10, 75)
setAlignLeft(objects.fovLabel, "FOV: ", 400 - 10, 85)
Core.Graphics.add(objects.hungerLabel)
Core.Graphics.add(objects.fovLabel)

Core.Graphics.OnNewFrame:Connect(function ()
    if Game.LocalPlayer.Loaded then
        objects.hungerLabel:setVisible(true)
        objects.fovLabel:setVisible(true)
        setAlignLeft(objects.hungerLabel, string.format("Hunger: %d", Game.LocalPlayer.CurrentHunger), 400 - 10, 75)
        setAlignLeft(objects.fovLabel, string.format("FOV: %f", Game.LocalPlayer.Camera.FOV), 400 - 10, 85)
    else
        objects.hungerLabel:setVisible(false)
        objects.fovLabel:setVisible(false)
    end
end)