local modPath = Core.getModpath("LunaCoreAPI")

CoreAPI.ItemGroups = {
    BUILDING_BOCKS = 1,
    DECORATION = 2,
    REDSTONE = 3,
    FOOD_MINERALS = 4,
    TOOLS = 5,
    POTIONS = 6,
    OTHERS = 7
}

dofile(modPath .. "/src/creative_inventory/creativeGroups.lua")