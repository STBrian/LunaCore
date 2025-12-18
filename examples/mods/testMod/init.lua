-- This template assumes it is used in conjunction with LunaCoreAPI

local LOGGER = CoreAPI.Utils.Logger.newLogger("testmod")

local testModReg = CoreAPI.Items.newItemRegistry("testmod") -- This must be the same as the mod name in mod.json
-- Core load mods names as lowercase, so use your mod name as lowercase

local COPPER_INGOT = testModReg:registerItem("copper_ingot", 254, {
    texture = "items/copper_ingot.3dst", -- This will search the texture in the folder assets/textures
    locales = {
        en_US = "Copper ingot",
        es_MX = "Lingote de cobre"
    }
})

local COPPER_PICKAXE = testModReg:registerItem("copper_pickaxe", 253, {
    texture = "items/copper_pickaxe.3dst",
    locales = {
        en_US = "Copper pickaxe",
        es_MX = "Pico de cobre"
    }
})
COPPER_PICKAXE.StackSize = 1

local AMETHYST_SHARD = testModReg:registerItem("amethyst_shard", 252, {
    texture = "items/amethyst_shard.3dst",
    locales = {
        en_US = "Amethyst shard",
        es_MX = "Fragmento de amatista"
    }
})

LOGGER:info(tostring(AMETHYST_SHARD ~= nil))

CoreAPI.ItemGroups.registerEntries(CoreAPI.ItemGroups.FOOD_MINERALS, function (entries)
    entries:addBefore(COPPER_INGOT, "iron_ingot")
    entries:addAfter(AMETHYST_SHARD, "iron_nugget")
end)

CoreAPI.ItemGroups.registerEntries(CoreAPI.ItemGroups.TOOLS, function (entries)
    entries:addAfter(COPPER_PICKAXE, "stone_pickaxe")
end)

testModReg:buildResources()

Game.Recipes.OnRegisterRecipes:Connect(function (recipesTable)
    local stick = Game.Items.findItemByName("stick")
    if stick and COPPER_PICKAXE and COPPER_INGOT then
        local stickIns = Game.Items.getItemInstance(stick, 1, 0)
        local copper_ingotIns = Game.Items.getItemInstance(COPPER_INGOT, 1, 0)
        local copper_pickaxe_ins = Game.Items.getItemInstance(COPPER_PICKAXE, 1, 0)
        Game.Recipes.registerShapedRecipe(recipesTable, copper_pickaxe_ins, 2, 50, "XXX", " # ", " # ", {{"X", copper_ingotIns}, {"#", stickIns}})
    end
end)