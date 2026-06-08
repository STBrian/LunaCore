local pluginFolder = Core.Menu.getMenuFolder():newFolder("MiniWorldEdit")
local pluginStorFolder = "data:/mwe_exports/"
if not Core.Filesystem.directoryExists(pluginStorFolder) then
    Core.Filesystem.createDirectory(pluginStorFolder)
end
local initialization = {}

local selectionItem = {id=280, data=1}
local started = false
local currentMode = nil

local selectionStart = nil
local selectionStartData = {}
local selectionEnd = nil
local selectionEndData = {}

local currentLoadedStructure = nil

--- "Just trust me".
--- Avoid the type mismatch warning. Make sure the param type is the desired one.
---@param param any
---@return any
local function unsafe(param)
    return param
end

local function containsInvalidChars(s)
    if string.find(s, "[^%w_]") then return true else return false end
end

local function isHolding(heldSlot, item)
    return not heldSlot:isEmpty() and heldSlot.Item.ID == item.id and heldSlot.ItemData == item.data
end

local function exportSelection(startPos, endPos)
    local name = Core.Keyboard.getString("Enter the name of the structure to export (press B to cancel): ")
    if name == nil then
        return
    end
    while containsInvalidChars(name) do
        name = Core.Keyboard.getString("Enter the name of the structure to export (press B to cancel): \nImportant! The name must only contain alphnumerical characters")
        if name == nil then
            return
        end
    end

    local startX = math.min(startPos.x, endPos.x)
    local endX = math.max(startPos.x, endPos.x)
    local startY = math.min(startPos.y, endPos.y)
    local endY = math.max(startPos.y, endPos.y)
    local startZ = math.min(startPos.z, endPos.z)
    local endZ = math.max(startPos.z, endPos.z)

    local data = {}
    for i = startY, endY, 1 do
        local y = i - startY + 1
        data[y] = {}
        for j = startX, endX, 1 do
            local x = j - startX + 1
            data[y][x] = {}
            for k = startZ, endZ, 1 do
                local z = k - startZ + 1
                local id, value = Game.World.getBlock(j, i, k)
                data[y][x][z] = {id, value}
            end
        end
    end

    local sizeY = math.abs(endY - startY) + 1
    local sizeX = math.abs(endX - startX) + 1
    local sizeZ = math.abs(endZ - startZ) + 1

    local originX = math.abs(startPos.x - startX)
    local originY = math.abs(startPos.y - startY)
    local originZ = math.abs(startPos.z - startZ)

    --- TODO: Export to a file
    currentLoadedStructure = {
        data=data,
        size={sizeX, sizeY, sizeZ},
        origin={originX, originY, originZ}
    }
end

local function pasteStructure(startPos, structdata)
    Async.run(function ()
        local blockCount = 0
        local data = structdata.data
        local origin = structdata.origin
        local sizeY = structdata.size[2]
        local sizeX = structdata.size[1]
        local sizeZ = structdata.size[3]
    
        for i = 0, sizeY - 1, 1 do
            for j = 0, sizeX - 1, 1 do
                for k = 0, sizeZ - 1, 1 do
                    local x, y, z = startPos.x + j - origin[1], startPos.y + i - origin[2], startPos.z + k - origin[3]
                    local id2, value2 = Game.World.getBlock(x, y, z)
                    local id, value = unpack(data[i+1][j+1][k+1])
                    if (id ~= id2 or value ~= value2) and id ~= 252 then
                        Game.World.setBlock(x, y, z, id, value)
                        blockCount = blockCount + 1
                        if blockCount > 20 then
                            Async.wait()
                            blockCount = 0
                        end
                    end
                end
            end
        end
    end)
end

local function actionListenerDisable()
    Async.run(function () -- run once this event is over, otherwise some problems can happen
        initialization.deinit()
    end)
end

local function actionListener()
    if Game.World.Loaded and Game.Gamepad.isPressed(Game.Gamepad.KeyCodes.L) then
        local heldSlot = Game.LocalPlayer.Inventory.Slots["hand"]

        local targetBlock = Game.World.getTargetBlock()
        if currentMode == "export" then
            local dyeId = 351
            local redData = 1
            local limeData = 10
            local redDyeItem = {id=dyeId, data=redData}
            local limeDyeItem = {id=dyeId, data=limeData}

            --- Selection
            if isHolding(heldSlot, selectionItem) then
                if targetBlock then
                    local diamondBlockId = 57
                    local redstoneBlockId = 152

                    if not selectionStart then
                        Game.World.message("Selected start: X=" .. targetBlock.x .. ", Y=" .. targetBlock.y .. ", Z=" .. targetBlock.z)
                        selectionStartData.id, selectionStartData.data = Game.World.getBlock(targetBlock.x, targetBlock.y, targetBlock.z)
                        Game.World.setBlock(targetBlock.x, targetBlock.y, targetBlock.z, diamondBlockId, 0)
                        selectionStart = {x=targetBlock.x, y=targetBlock.y, z=targetBlock.z}
                    else
                        if selectionEnd then
                            Game.World.setBlock(selectionEnd.x, selectionEnd.y, selectionEnd.z, selectionEndData.id, selectionEndData.data)
                        end
                        Game.World.message("Selected end: X=" .. targetBlock.x .. ", Y=" .. targetBlock.y .. ", Z=" .. targetBlock.z)
                        selectionEndData.id, selectionEndData.data = Game.World.getBlock(targetBlock.x, targetBlock.y, targetBlock.z)
                        Game.World.setBlock(targetBlock.x, targetBlock.y, targetBlock.z, redstoneBlockId, 0)
                        selectionEnd = {x=targetBlock.x, y=targetBlock.y, z=targetBlock.z}
                    end
                else
                    Game.World.message("Not a block")
                end

            --- Accept
            elseif isHolding(heldSlot, limeDyeItem) then
                if selectionStart and selectionEnd then
                    Game.World.setBlock(selectionStart.x, selectionStart.y, selectionStart.z, selectionStartData.id, selectionStartData.data)
                    Game.World.setBlock(selectionEnd.x, selectionEnd.y, selectionEnd.z, selectionEndData.id, selectionEndData.data)
                    exportSelection(selectionStart, selectionEnd)
                    actionListenerDisable()
                else
                    Game.World.message("The selection is incomplete")
                end

            --- Cancel/Undo
            elseif isHolding(heldSlot, redDyeItem) then
                if selectionEnd then
                    Game.World.setBlock(selectionEnd.x, selectionEnd.y, selectionEnd.z, selectionEndData.id, selectionEndData.data)
                    selectionEnd = nil
                elseif selectionStart then
                    Game.World.setBlock(selectionStart.x, selectionStart.y, selectionStart.z, selectionStartData.id, selectionStartData.data)
                    selectionStart = nil
                else
                    actionListenerDisable()
                end
            end

        elseif currentMode == "paste" then
            local dyeId = 351
            local redData = 1
            local limeData = 10
            local redDyeItem = {id=dyeId, data=redData}
            local limeDyeItem = {id=dyeId, data=limeData}

            --- Selection
            if isHolding(heldSlot, selectionItem) then
                if targetBlock then
                    selectionStart = {x=targetBlock.x, y=targetBlock.y, z=targetBlock.z}
                    pasteStructure(selectionStart, currentLoadedStructure)
                else
                    Game.World.message("Not a block")
                end

            --- Cancel/Undo
            elseif isHolding(heldSlot, redDyeItem) then
                actionListenerDisable()
            end
        end
    end
end

pluginFolder:newEntry("Export", function ()
    if Game.World.Loaded then
        if started then
            Core.Menu.showMessageBox("Another mode is already enabled")
            return
        end
        if Game.LocalPlayer.Gamemode ~= 1 then
            local stickItem = Game.Items.findItemByID(selectionItem.id)
            local dyeItem = Game.Items.findItemByID(351)
            local fillBlock = Game.Items.findItemByID(252)
            local limeData = 10
            local redData = 1
            Async.run(function ()
                -- Execute this code once the menu is closed
                -- otherwise this can cause a deadlock

                local firstSlot = Game.LocalPlayer.Inventory.Slots[0]
                local secondSlot = Game.LocalPlayer.Inventory.Slots[1]
                local thirdSlot = Game.LocalPlayer.Inventory.Slots[2]
                local fourthSlot = Game.LocalPlayer.Inventory.Slots[3]
                firstSlot:setItem(unsafe(stickItem), selectionItem.data)
                secondSlot:setItem(unsafe(dyeItem), limeData) -- Accept item
                thirdSlot:setItem(unsafe(dyeItem), redData) -- Cancel item
                fourthSlot:setItem(unsafe(fillBlock), 1)
                firstSlot.ItemCount = 1
                secondSlot.ItemCount = 1
                thirdSlot.ItemCount = 1
            end)
            started = true
            currentMode = "export"
            selectionStart = nil
            selectionEnd = nil
            Game.Gamepad.OnKeyPressed:Connect(actionListener)
            Core.Menu.showMessageBox("Close the menu")
        else
            Core.Menu.showMessageBox("Only available in creative worlds")
        end
    else
        Core.Menu.showMessageBox("First enter a world")
    end
end)

pluginFolder:newEntry("Paste", function ()
    if Game.World.Loaded then
        if started then
            Core.Menu.showMessageBox("Another mode is already enabled")
            return
        end
        if Game.LocalPlayer.Gamemode ~= 1 then
            if currentLoadedStructure == nil then
                Core.Menu.showMessageBox("No structure is loaded yet")
                return
            end
            local stickItem = Game.Items.findItemByID(selectionItem.id)
            local dyeItem = Game.Items.findItemByID(351)
            local limeData = 10
            local redData = 1
            Async.run(function ()
                -- Execute this code once the menu is closed
                -- otherwise this can cause a deadlock

                local firstSlot = Game.LocalPlayer.Inventory.Slots[0]
                local secondSlot = Game.LocalPlayer.Inventory.Slots[1]
                local thirdSlot = Game.LocalPlayer.Inventory.Slots[2]
                firstSlot:setItem(unsafe(stickItem), selectionItem.data)
                secondSlot:setItem(unsafe(dyeItem), limeData) -- Accept item
                thirdSlot:setItem(unsafe(dyeItem), redData) -- Cancel item
                firstSlot.ItemCount = 1
                secondSlot.ItemCount = 1
                thirdSlot.ItemCount = 1
            end)
            started = true
            currentMode = "paste"
            Game.Gamepad.OnKeyPressed:Connect(actionListener)
            Core.Menu.showMessageBox("Close the menu")
        else
            Core.Menu.showMessageBox("Only available in creative worlds")
        end
    else
        Core.Menu.showMessageBox("First enter a world")
    end
end)

function initialization.deinit()
    if started then
        Game.Gamepad.OnKeyPressed:Disconnect(actionListener)
        started = false
    end
end

Game.World.OnWorldLeave:Connect(function ()
    initialization.deinit()
end)