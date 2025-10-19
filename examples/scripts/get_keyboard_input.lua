local Gamepad = Game.Gamepad
local Keycodes = Gamepad.KeyCodes
local Keyboard = Core.Keyboard
local Debug = Core.Debug

-- This will open a keyboard on number mode
-- If no input it will return nil
-- It will show the message "Enter a number: " on top screen
Game.Gamepad.OnKeyPressed:Connect(function ()
    if Gamepad.isDown(Keycodes.ZL) and Gamepad.isDown(Keycodes.ZR) then
        local input = Keyboard.getNumber("Enter a number: ")
        Debug.message(tostring(input))
    end
end)

-- This will open a keyboard on string mode (all alphabetical and special characters)
-- If no input it will return nil
-- It will show on top screen the message "Enter your message: "
Game.Gamepad.OnKeyPressed:Connect(function ()
    if Gamepad.isDown(Keycodes.L) then
        local input = Keyboard.getString("Enter your message: ")
        if input then
            Debug.message(input)
        end
    end
end)

-- This will open a keyboard on hexadecimal mode and return as integer
-- The keyboard won't show any message on top screen
-- You can print the input as hexadecimal using string.format("%X", input)
Game.Gamepad.OnKeyPressed:Connect(function ()
    if Gamepad.isDown(Keycodes.R) then
        local input = Keyboard.getHex()
        Debug.message(tostring(string.format("%X", input)))
    end
end)