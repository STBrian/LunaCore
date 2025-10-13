#pragma once

#include <string>
#include "Game/game_utils/custom_string.hpp"
#include "Game/world/item/Item.hpp"

namespace Game {
    class ItemRecipe {
        public:

        typedef struct {
            Item* item = nullptr;
            void* block = nullptr;
            ItemInstance ins;
            char id;
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
            short material_ids[5] = {
                *(unsigned char*)(*(int*)0x00a34740 + 4), // Prob wood block
                *(unsigned char*)(*(int*)0x00a3473c + 4), // Prob cobblestone block
                Item::mIronIngot->itemId,
                Item::mDiamond->itemId,
                Item::mGoldIngot->itemId
            };
            Item* tool_items[20] = {

            }; // Imagine the items are here
            void** unknownArray;
            reinterpret_cast<void**(*)(void**&, void*, int)>(unknownArray, (void*)0x00989038, 80);
            for (int i = 0; i < 5; i++) { // materials
                short materialId = material_ids[i];
                for (int j = 0; j < 4; j++) { // recipes
                    Item* curTool = tool_items[j * 5 + i];
                    if (materialId < 256) { // block
                        TestRecipeStruct stick, material;
                        stick.item = Item::mStick;
                        stick.id = '#';
                        material.block = Item::mBlocks[materialId];
                        material.id = 'X';
                        AnotherTestStruct vec;
                        // [int, int, int]*, [Item*, int]
                        reinterpret_cast<void(*)(AnotherTestStruct&, TestRecipeStruct&)>(0x008ff20c)(vec, stick);
                        reinterpret_cast<void(*)(TestRecipeStruct&)>(0x00638c28)(stick);
                        reinterpret_cast<void(*)(AnotherTestStruct&, TestRecipeStruct&)>(0x008ff20c)(vec, material);
                        reinterpret_cast<void(*)(TestRecipeStruct&)>(0x00638c28)(material);
                        ItemInstance targetTool(curTool);
                        void* unknownValue = unknownArray[j * 5 + i];
                        reinterpret_cast<void(*)(void*, ItemInstance&, ItemRecipe&, AnotherTestStruct&, int, void*)>(0x0063684c)(ptr1, targetTool, recipes[j], vec, 2, unknownValue);
                        for (void** unk1 = (void**)targetTool.field12; unk1 != (void**)targetTool.field13; unk1++) {
                        }
                        reinterpret_cast<void(*)(void*)>(0x001007d0)((void*)targetTool.field12);
                        for (void** unk1 = (void**)targetTool.field9; unk1 != (void**)targetTool.field10; unk1++) {
                        }
                        reinterpret_cast<void(*)(void*)>(0x001007d0)((void*)targetTool.field9);
                        if ((void*)targetTool.field4 != nullptr) {
                            (*reinterpret_cast<void(**)(void)>(*(int*)targetTool.field4 + 4))();
                        }
                    } else { // item
                        AnotherTestStruct stest2;
                        TestRecipeStruct stest;
                    }
                }
            }
        }
    };
}