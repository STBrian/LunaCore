Core.Debug.log("Init testMod", false)

local testModReg = CoreAPI.Items.newItemRegistry("testMod")

testModReg:registerItem("copper_ingot", 254, {
    texture = "items/copper_ingot.3dst",
    group = CoreAPI.ItemGroups.newItemGroupIdentifier(CoreAPI.ItemGroups.FOOD_MINERALS, CoreAPI.ItemGroups.beforeItem("iron_ingot")),
    locales = {
        en_US = "Copper ingot",
        es_MX = "Lingote de cobre"
    }
})

testModReg:registerItem("copper_ingot2", 253, {
    texture = "items/copper_ingot.3dst",
    group = CoreAPI.ItemGroups.newItemGroupIdentifier(CoreAPI.ItemGroups.FOOD_MINERALS, CoreAPI.ItemGroups.afterItem("testmod:copper_ingot")),
    locales = {
        en_US = "Better copper ingot",
        es_MX = "Lingote de cobre piola"
    }
})

testModReg:registerItems()