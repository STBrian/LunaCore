-- This template assumes it is used in conjunction with LunaCoreAPI

local LOGGER = CoreAPI.Utils.Logger.newLogger("testMod")

local testModReg = CoreAPI.Items.newItemRegistry("testMod") -- This must be the same as the mod name in mod.json

local COPPER_INGOT = testModReg:registerItem("copper_ingot", 254, {
    texture = "items/copper_ingot.3dst", -- This will search the texture in the folder assets/textures
    group = {CoreAPI.ItemGroups.FOOD_MINERALS, CoreAPI.ItemGroups.Creative.beforeItem("iron_ingot")},
    locales = {
        en_US = "Copper ingot",
        es_MX = "Lingote de cobre"
    }
})

local COPPER_INGOT2 = testModReg:registerItem("copper_ingot2", 253, {
    texture = "items/copper_ingot.3dst",
    --group = {CoreAPI.ItemGroups.FOOD_MINERALS, CoreAPI.ItemGroups.Creative.afterItem("testmod:copper_ingot")},
    stackSize = 32,
    locales = {
        en_US = "Better copper ingot",
        es_MX = "Lingote de cobre piola"
    }
})

local AMETHYST_SHARD = testModReg:registerItem("amethyst_shard", 252, {
    texture = "items/amethyst_shard.3dst",
    group = {CoreAPI.ItemGroups.FOOD_MINERALS, CoreAPI.ItemGroups.Creative.afterItem("iron_nugget")},
    locales = {
        en_US = "Amethyst shard",
        es_MX = "Fragmento de amatista"
    }
})

LOGGER:info(tostring(AMETHYST_SHARD ~= nil))

testModReg:buildResources()