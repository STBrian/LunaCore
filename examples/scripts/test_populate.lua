local plgMenuFldr = Core.Menu.getMenuFolder()

plgMenuFldr:newEntry("Test populate", function ()
    local options = {
        "Me ama",
        "No me ama",
        "Me ama más o menos"
    }
    local selection = Core.Keyboard.populate(options)
    Core.Debug.message(options[selection])
end)