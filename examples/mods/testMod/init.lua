Core.Debug.log("Init testMod", false)

local testModReg = CoreAPI.Items.newItemRegistry("testMod") -- This must be the same as the mod name in mod.json

testModReg:registerItem("copper_ingot", 254, {
    texture = "items/copper_ingot.3dst", -- This will search the texture in the folder assets/textures
    group = {CoreAPI.ItemGroups.FOOD_MINERALS, CoreAPI.ItemGroups.Creative.beforeItem("iron_ingot")},
    locales = {
        en_US = "Copper ingot",
        es_MX = "Lingote de cobre"
    }
})

testModReg:registerItem("copper_ingot2", 253, {
    texture = "items/copper_ingot.3dst",
    --group = {CoreAPI.ItemGroups.FOOD_MINERALS, CoreAPI.ItemGroups.Creative.afterItem("testmod:copper_ingot")},
    stackSize = 32,
    locales = {
        en_US = "Better copper ingot",
        es_MX = "Lingote de cobre piola"
    }
})

testModReg:registerItem("amethyst_shard", 252, {
    texture = "items/amethyst_shard.3dst",
    group = {CoreAPI.ItemGroups.FOOD_MINERALS, CoreAPI.ItemGroups.Creative.afterItem("iron_nugget")},
    locales = {
        en_US = "Amethyst shard",
        es_MX = "Fragmento de amatista"
    }
})


testModReg:registerItems()