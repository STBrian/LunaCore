local modpath = Core.getModpath("LunaCoreAPI")

CoreAPI = {_VERSION = "0.1.0"}

CoreAPI.Utils = {}
dofile(modpath .. "/src/utils/string.lua")
dofile(modpath .. "/src/utils/table.lua")
---@type JsonClass
CoreAPI.Utils.Json = dofile(modpath .. "/src/utils/json.lua")
---@type StructClass
CoreAPI.Utils.Struct = dofile(modpath .. "/src/utils/struct.lua")
---@type ClassicClass
CoreAPI.Utils.Classic = dofile(modpath .. "/src/utils/classic.lua")
---@type BitOpClass
CoreAPI.Utils.Bitop = dofile(modpath .. "/src/utils/bitop/funcs.lua")

CoreAPI.ResourcePacks = {}
CoreAPI.ResourcePacks.vanilla = {hash = 0x79954554, res = 16}


CoreAPI.Languages = {"de_DE", "en_GB", "en_US", "es_ES", "es_MX", "fr_CA",
    "fr_FR", "it_IT", "ja_JP", "ko_KR", "nl_NL", "pt_BR", "pt_PT", "ru_RU",
    "zh_CN", "zh_TW"}

dofile(modpath .. "/src/items.lua")
dofile(modpath .. "/src/itemGroups.lua")

Core.Debug.log("Loaded LunaCoreAPI", false);