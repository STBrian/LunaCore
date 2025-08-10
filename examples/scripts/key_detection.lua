local keycodes = Game.Gamepad.KeyCodes
local Debug = Core.Debug

-- This will print the keycode of the pressed key(s)
Game.Event.OnKeyPressed:Connect(function (keys)
    Debug.message("Pressed keycode: " .. keys)
end)

-- This will be executed only when key A is pressed
Game.Event.OnKeyPressed:Connect(function ()
    if Game.Gamepad.isPressed(keycodes.A) then
        Debug.message("Pressed key A")
    end
end)

-- This will be executed only when key A is released
Game.Event.OnKeyReleased:Connect(function ()
    if Game.Gamepad.isReleased(keycodes.A) then
        Debug.message("Released key A")
    end
end)

-- This will be executed while key B is down
Game.Event.OnKeyDown:Connect(function ()
    if Game.Gamepad.isPressed(keycodes.B) then
        Debug.message("Key B is down")
    end
end)

-- This will be executed until key DPADDOWN is down
Async.create(function ()
    local continue = true
    while continue and Async.wait() do
        if Game.Gamepad.isDown(keycodes.DPADDOWN) then
            continue = false
        end
    end
    Core.Debug.message("Ended async task")
end)