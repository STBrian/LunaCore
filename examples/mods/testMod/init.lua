-- This template assumes it is used in conjunction with LunaCoreAPI

local LOGGER = CoreAPI.Utils.Logger.newLogger("testMod")

local testModReg = CoreAPI.Items.newItemRegistry("testMod") -- This must be the same as the mod name in mod.json

local COPPER_INGOT = testModReg:registerItem("copper_ingot", 254, {
    texture = "items/copper_ingot.3dst", -- This will search the texture in the folder assets/textures
    locales = {
        en_US = "Copper ingot",
        es_MX = "Lingote de cobre"
    }
})

local COPPER_INGOT2 = testModReg:registerItem("copper_ingot2", 253, {
    texture = "items/copper_ingot.3dst",
    locales = {
        en_US = "Better copper ingot",
        es_MX = "Lingote de cobre piola"
    }
})
COPPER_INGOT2.StackSize = 32

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
    entries:add(COPPER_INGOT2)
    entries:addAfter(AMETHYST_SHARD, "iron_nugget")
end)

testModReg:buildResources()