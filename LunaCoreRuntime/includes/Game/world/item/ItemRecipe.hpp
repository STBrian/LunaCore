#pragma once

#include <string>
#include "Game/game_utils/custom_string.hpp"
#include "Game/world/item/Item.hpp"

namespace Game {
    class ItemRecipe {
        public:

        typedef struct {
            Item* item1;
            Item* item2;
            ItemInstance ins;
        } TestRecipeStruct;

        typedef struct {
            int a = 0, b = 0, c = 0;
        } AnotherTestStruct;

        void* line1;
        void* line2;
        void* line3;

        ItemRecipe(const CustomString& line1, const CustomString& line2, const CustomString& line3) {
            reinterpret_cast<void(*)(ItemRecipe*, const CustomString&, const CustomString&, const CustomString&)>(0x641144)(this, line1, line2, line3);
        }

        static void registerRecipe(const std::string& line1, const std::string& line2, const std::string& line3) {
            ItemRecipe recipe(line1, line2, line3);
        }

        static void testRegisterRecipe(void* ptr1) {
            ItemRecipe recipes[4] = {
                {"XXX", // pickaxe
                " # ",
                " # "},
                {"X", // shovel
                "#",
                "#"},
                {"XX", // axe
                "X#",
                " #"},
                {"XX", // hoe
                " #",
                " #"},
            };
            short material_id[5]; // Imagine the ids are here
            Item* tool_items[20]; // Imagine the items are here
            // Here another function is called but idk what it does
            int i = 0;
            do {
                int j = 0;
                short curMaterial = material_id[i];
                Item* materialItem = Item::mItems[curMaterial];
                do {
                    Item* curTool = tool_items[j * 5 + i];
                    if (curMaterial < 256) { // block
                        TestRecipeStruct stest;
                        // stest.item1 = stick_item 
                        stest.item2 = nullptr;
                        stest.ins.field15 = '#';
                        AnotherTestStruct stest2;
                        // [int, int, int]*, [Item*, int]
                        reinterpret_cast<void(*)(AnotherTestStruct*, TestRecipeStruct*)>(0x008ff20c)(&stest2, &stest);
                        reinterpret_cast<void(*)(TestRecipeStruct*)>(0x00638c28)(&stest);
                        stest.item1 = nullptr;
                        stest.item2 = materialItem;
                        stest.ins = ItemInstance();
                        reinterpret_cast<void(*)(AnotherTestStruct*, TestRecipeStruct*)>(0x008ff20c)(&stest2, &stest);
                        reinterpret_cast<void(*)(TestRecipeStruct*)>(0x00638c28)(&stest);
                        ItemInstance targetTool(curTool);
                        reinterpret_cast<void(*)(void*, ItemInstance*, ItemRecipe*, AnotherTestStruct*, int, TestRecipeStruct*)>(0x0063684c)(ptr1, &targetTool, &recipes[j], &stest2, 2, &stest);
                        int a = stest2.a;
                        int b = stest2.b;
                    } else { // item
                        AnotherTestStruct stest2;
                        TestRecipeStruct stest;
                    }
                } while (j < 4);
            } while (i < 5);
        }
    };
}