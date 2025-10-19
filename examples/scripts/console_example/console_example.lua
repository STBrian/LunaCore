local stringUtils = require("utils.string_utils")
local gamepad = Game.Gamepad
local keys = gamepad.KeyCodes
local console = {
    maxlines = 10,
    lines = {},
    env = {},
    bg = Core.Graphics.colorRGB(0, 0, 0),
    fg = Core.Graphics.colorRGB(255, 255, 255),
    openKey = keys.START,
    frames = 0,
    firstframe = true
}
setmetatable(console.env, {__index = _G})

local function printToConsole(text)
    table.insert(console.lines, 1, text)
    if #console.lines > console.maxlines then
        table.remove(console.lines, #console.lines)
    end
end

local function eval_line(input)
    local chunk, err = loadstring("return "..input)
    if not chunk then
        chunk, err = loadstring(input)
    end

    if not chunk then
        printToConsole("lua: "..err)
    else
        setfenv(chunk, console.env)

        local results = {pcall(chunk)}
        if not results[1] then
            printToConsole("lua: "..results[2])
        else
            for i = 2, #results do
                printToConsole(tostring(results[i]))
            end
        end
    end
end

---@param command string
local function processCommand(command)
    local args = stringUtils.split(command, " ")
    if args[1] == "lua" then
        local code = command:sub(#args[1]+2)
        eval_line(code)
    elseif args[1] == "gamemode" then
        if Game.World.Loaded then
            if args[2] == 0 or args[2] == "survival" or args[2] == "s" then
                Game.LocalPlayer.Gamemode = 0
                printToConsole("gamemode: changed to survival")
            elseif args[2] == 1 or args[2] == "creative" or args[2] == "c" then
                Game.LocalPlayer.Gamemode = 1
                printToConsole("gamemode: changed to creative")
            elseif args[2] == 2 or args[2] == "adventure" or args[2] == "a" then
                Game.LocalPlayer.Gamemode = 2
                printToConsole("gamemode: changed to adventure")
            else
                printToConsole("gamemode: invalid gamemode")
            end
        else
            printToConsole("gamemode: not in a world")
        end
    elseif args[1] == "exit" then
        Core.Graphics.close()
    end
end

local function drawGraphics(screen)
    if screen == "top" then
        Core.Graphics.drawRectFill(20, 20, 360, 200, console.bg)
        Core.Graphics.drawText("Command console. Press B to close", 25, 25, console.fg);
        local y = 200
        for index, value in ipairs(console.lines) do
            Core.Graphics.drawText(value, 25, y, console.fg);
            y = y - 15
        end
    else
        console.frames = console.frames + 1
        if console.frames % 2 == 0 then -- Execute only when both framebuffers were drawn
            if console.firstframe then -- Avoid execute when first frame
                console.firstframe = false
                return
            end
            if console.frames > 0xFF then
                console.frames = 0
            end
            local command = Core.Keyboard.getString()
            if command == nil then
                printToConsole("> exit")
                Core.Graphics.close()
            else
                printToConsole(string.format("> %s", command))
                processCommand(command)
            end
        end
    end
end

-- You need to call open function to be able to draw on screen
-- The game will frezee and that allows to avoid artifacts on screen
-- Every other events and async task will continue execution
Game.Gamepad.OnKeyReleased:Connect(function ()
    if gamepad.isReleased(console.openKey) and Game.World.Loaded then
        if not Core.Graphics.isOpen() then
            console.firstframe = true
            Core.Graphics.open(drawGraphics)
        end
    end
end)