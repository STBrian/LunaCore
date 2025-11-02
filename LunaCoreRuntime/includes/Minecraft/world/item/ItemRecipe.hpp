#pragma once

#include <string>
#include "Minecraft/game_utils/custom_string.hpp"
#include "Minecraft/game_utils/generic_vector.hpp"
#include "Minecraft/world/item/Item.hpp"
#include "Minecraft/world/item/Block.hpp"
#include "Minecraft/world/item/Recipes.hpp"

namespace Minecraft {
    class ItemRecipe {
        public:
        using Item = Game::Item;
        using ItemInstance = Game::ItemInstance;

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

        static void testCustomRecipe(void* ptr1) {
            Recipes::Shape recipe = {"XXX", " # ", " # "};
            GenericVector vec;
            definition(vec, '#', Item::mEmerald, 'X', Block::mBlocks[(*reinterpret_cast<Block**>(0x00a34740))->blockId]);
            ItemInstance targetItem(Item::mItems[57]); // Diamond block
            Recipes::addShapedRecipe(ptr1, targetItem, recipe, vec, 2, 10);

            // --- Clean ---
            // I reused a clean routine from InternalRecipeElementDefinition destructor, just adjusted the offset
            reinterpret_cast<void(*)(ItemInstance*)>(0x00638c28)((ItemInstance*)((u32)&targetItem - 0x8));
            // Clean elements of vec
            for (InternalRecipeElementDefinition* current = reinterpret_cast<InternalRecipeElementDefinition*>(vec.field1); current != reinterpret_cast<InternalRecipeElementDefinition*>(vec.field2); current = current + 1) {
                InternalRecipeElementDefinition::ManualDestroy(current);
            }
        }

        static void testRegisterRecipe(void* ptr1) {
            Recipes::Shape recipes[4] = {
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
            unsigned short material_ids[5] = {
                ((*reinterpret_cast<Block**>(0x00a34740))->blockId), // Prob wood block
                ((*reinterpret_cast<Block**>(0x00a3473c))->blockId), // Prob cobblestone block
                Item::mIronIngot->itemId,
                Item::mDiamond->itemId,
                Item::mGoldIngot->itemId
            };
            Item* tool_items[20] = {
                Item::mPickAxe_wood,
                Item::mPickAxe_stone,
                Item::mPickAxe_iron,
                Item::mPickAxe_diamond,
                Item::mPickAxe_gold,
                Item::mShovel_wood,
                Item::mShovel_stone,
                Item::mShovel_iron,
                Item::mShovel_diamond,
                Item::mShovel_gold,
            }; // Imagine the items are here
            int unknownArray[20];
            reinterpret_cast<void**(*)(int*, void*, int)>(unknownArray, (void*)0x00989038, 80);
            for (int i = 0; i < 5; i++) { // materials
                short materialId = material_ids[i];
                for (int j = 0; j < 4; j++) { // recipes
                    Item* curTool = tool_items[j * 5 + i];
                    if (materialId < 256) { // block
                        GenericVector vec;
                        definition(vec, '#', Item::mStick, 'X', Block::mBlocks[materialId]);
                        ItemInstance targetTool(curTool);
                        int unknownValue = unknownArray[j * 5 + i]; // Maybe position in recipes vector?
                        Recipes::addShapedRecipe(ptr1, targetTool, recipes[j], vec, 2, unknownValue);
                        
                        // I reused a clean routine from InternalRecipeElementDefinition destructor, just adjusted the offset
                        reinterpret_cast<void(*)(ItemInstance*)>(0x00638c28)((ItemInstance*)((u32)&targetTool - 0x8));
                        for (InternalRecipeElementDefinition* current = reinterpret_cast<InternalRecipeElementDefinition*>(vec.field1); current != reinterpret_cast<InternalRecipeElementDefinition*>(vec.field2); current++) {
                            InternalRecipeElementDefinition::ManualDestroy(current);
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