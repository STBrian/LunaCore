-- Someone should put more effort into examples
local mainFolder = Core.Menu.getMenuFolder()
local nfolder = mainFolder:newFolder("test1")
nfolder:newEntry("test1", function ()
    Core.Debug.log("hi from test1")
end)