#pragma once

#include "game/gstd/gstd_vector.hpp"
#include "game/gstd/gstd_string.hpp"
#include "game/world/item/Item.hpp"
#include "game/world/level/block/Block.hpp"

namespace Minecraft {
    using Item = Game::Item;
    using Block = Minecraft::Block;
    using ItemInstance = Game::ItemInstance;

    class InternalRecipeElementDefinition {
        public:
        Item* item = nullptr;
        void* block = nullptr;
        ItemInstance ins;
        char id;
    };

    typedef struct {
        char id;
        Item* item = nullptr;
        Block* block = nullptr;
    } RecipeComponentDef;

    typedef struct {
        char id;
        ItemInstance* item = nullptr;
    } RecipeComponentDefIns;

    // Item
    void definition(gstd::vector<InternalRecipeElementDefinition>& vec, char itemchr, Item* item) {
        #if __STDC_HOSTED__
        vec.push_back((InternalRecipeElementDefinition){.item=item, .id=itemchr});
        #else
        InternalRecipeElementDefinition itemDef;
        itemDef.item = item;
        itemDef.id = itemchr;
        reinterpret_cast<void(*)(gstd::vector<InternalRecipeElementDefinition>&, InternalRecipeElementDefinition&)>(0x008ff20c)(vec, itemDef); // emplace_back
        #endif
    }

    // ItemInstance
    void definition(gstd::vector<InternalRecipeElementDefinition>& vec, char itemchr, ItemInstance* item) {
        #if __STDC_HOSTED__
        vec.push_back((InternalRecipeElementDefinition){.ins=item, .id=itemchr});
        #else
        InternalRecipeElementDefinition itemDef = {nullptr, nullptr, ItemInstance(item), itemchr};
        reinterpret_cast<void(*)(gstd::vector<InternalRecipeElementDefinition>&, InternalRecipeElementDefinition&)>(0x008ff20c)(vec, itemDef); // emplace_back
        #endif
    }

    // Block
    void definition(gstd::vector<InternalRecipeElementDefinition>& vec, char blockchr, Block* block) {
        #if __STDC_HOSTED__
        vec.push_back((InternalRecipeElementDefinition){.block=block, .id=blockchr});
        #else
        InternalRecipeElementDefinition blockDef;
        blockDef.block = block;
        blockDef.id = blockchr;
        reinterpret_cast<void(*)(gstd::vector<InternalRecipeElementDefinition>&, InternalRecipeElementDefinition&)>(0x008ff20c)(vec, blockDef); // emplace_back
        #endif       
    }

    // Item, Block
    void definition(gstd::vector<InternalRecipeElementDefinition>& vec, char itemchr, Item* item, char blockchr, Block* block) {
        definition(vec, itemchr, item);
        definition(vec, blockchr, block);
    }

    // Use an array of components definitions
    void definition(gstd::vector<InternalRecipeElementDefinition>& vec, RecipeComponentDef* components, u32 size) {
        for (u32 i = 0; i < size; i++) {
            if (components[i].item != nullptr) {
                definition(vec, components[i].id, components[i].item);
            } else if (components[i].block != nullptr) {
                definition(vec, components[i].id, components[i].block);
            }
        }
    }

    // Use an array of components definitions
    void definition(gstd::vector<InternalRecipeElementDefinition>& vec, RecipeComponentDefIns* components, u32 size) {
        for (u32 i = 0; i < size; i++) {
            if (components[i].item != nullptr) {
                definition(vec, components[i].id, components[i].item);
            }
        }
    }

    class Recipes {
        public:

        class Shape {
            public:
            gstd::GenericVector vec;

            Shape(const gstd::string& line1, const gstd::string& line2, const gstd::string& line3) {
                reinterpret_cast<void(*)(Shape*,const gstd::string&,const gstd::string&,const gstd::string&)>(0x00641144)(this, line1, line2, line3);
            }

            ~Shape() {
                reinterpret_cast<void(*)(Shape*)>(0x00902fa4)(this);
            }
        };

        static void addShapedRecipe(void* ptr1, ItemInstance& result, Recipes::Shape& shape, gstd::GenericVector& defVector, int categoryId, int pos) {
            reinterpret_cast<void(*)(void*, ItemInstance&, Recipes::Shape&, gstd::GenericVector&,int,int)>(0x0063684c)(ptr1, result, shape, defVector, categoryId, pos);
        }

        static void addShapedRecipe(void* ptr1, ItemInstance& result, Recipes::Shape& shape, gstd::vector<InternalRecipeElementDefinition>& defVector, int categoryId, int pos) {
            reinterpret_cast<void(*)(void*,ItemInstance&,Recipes::Shape&,gstd::vector<InternalRecipeElementDefinition>&,int,int)>(0x0063684c)(ptr1, result, shape, defVector, categoryId, pos);
        }

        static void addShapedRecipe(void* ptr1, ItemInstance& result, const gstd::string& line1, gstd::vector<InternalRecipeElementDefinition>& defVector, int categoryId, int pos) {
            reinterpret_cast<void(*)(void*,ItemInstance&,const gstd::string&,gstd::vector<InternalRecipeElementDefinition>&,int,int)>(0x006366dc)(ptr1, result, line1, defVector, categoryId, pos);
        }

        static void addShapedRecipe(void* ptr1, ItemInstance& result, const gstd::string& line1, const gstd::string& line2, gstd::vector<InternalRecipeElementDefinition>& defVector, int categoryId, int pos) {
            reinterpret_cast<void(*)(void*,ItemInstance&,const gstd::string&,const gstd::string&,gstd::vector<InternalRecipeElementDefinition>&,int,int)>(0x00636744)(ptr1, result, line1, line2, defVector, categoryId, pos);
        }

        static void registerRecipe(void* ptr1, Item* resItem, const char* line1, const char* line2, const char* line3, RecipeComponentDef* components, u32 size, u16 categoryId, s16 position) {
            Shape recipe = {line1, line2, line3};
            gstd::vector<InternalRecipeElementDefinition> vec;
            definition(vec, components, size);
            ItemInstance targetItem(resItem);
            Recipes::addShapedRecipe(ptr1, targetItem, recipe, vec, categoryId, position);
        }

        static void testCustomRecipe(void* ptr1) {
            Recipes::Shape recipe = {"XXX", " # ", " # "};
            gstd::vector<InternalRecipeElementDefinition> vec;
            definition(vec, '#', Item::mEmerald, 'X', Block::mBlocks[(*reinterpret_cast<Block**>(0x00a34740))->blockId]);
            ItemInstance targetItem(Item::mItems[57]); // Diamond block
            Recipes::addShapedRecipe(ptr1, targetItem, recipe, vec, 2, 10);
        }
    };
}