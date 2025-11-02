#pragma once

#include "Minecraft/game_utils/generic_vector.hpp"
#include "Minecraft/game_utils/custom_string.hpp"
#include "Minecraft/world/item/Item.hpp"
#include "Minecraft/world/item/Block.hpp"

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

        InternalRecipeElementDefinition() {}
        ~InternalRecipeElementDefinition() {
            ManualDestroy(this);
        }

        static void ManualDestroy(InternalRecipeElementDefinition* ins) {
            reinterpret_cast<void(*)(InternalRecipeElementDefinition*)>(0x00638c28)(ins);
        }
    };

    typedef struct {
        char id;
        Item* item = nullptr;
        Block* block = nullptr;
    } RecipeComponentDef;

    // Item
    void definition(GenericVector& vec, char itemchr, Item* item) {
        InternalRecipeElementDefinition itemDef;
        itemDef.item = item;
        itemDef.id = itemchr;
        reinterpret_cast<void(*)(GenericVector&, InternalRecipeElementDefinition&)>(0x008ff20c)(vec, itemDef); // emplace_back
    }

    // Block
    void definition(GenericVector& vec, char blockchr, Block* block) {
        InternalRecipeElementDefinition blockDef;
        blockDef.block = block;
        blockDef.id = blockchr;
        reinterpret_cast<void(*)(GenericVector&, InternalRecipeElementDefinition&)>(0x008ff20c)(vec, blockDef); // emplace_back
    }

    // Item, Block
    void definition(GenericVector& vec, char itemchr, Item* item, char blockchr, Block* block) {
        definition(vec, itemchr, item);
        definition(vec, blockchr, block);
    }

    // Use an array of components definitions
    void definition(GenericVector& vec, RecipeComponentDef* components, u32 size) {
        for (u32 i = 0; i < size; i++) {
            if (components[i].item != nullptr) {
                definition(vec, components[i].id, components[i].item);
            } else if (components[i].block != nullptr) {
                definition(vec, components[i].id, components[i].block);
            }
        }
    }

    class Recipes {
        public:

        class Shape {
            public:
            GenericVector vec;

            Shape(const CustomString& line1, const CustomString& line2, const CustomString& line3) {
                reinterpret_cast<void(*)(Shape*,const CustomString&,const CustomString&,const CustomString&)>(0x00641144)(this, line1, line2, line3);
            }

            ~Shape() {
                reinterpret_cast<void(*)(Shape*)>(0x00902fa4)(this);
            }
        };

        static void addShapedRecipe(void* ptr1, ItemInstance& result, Recipes::Shape& shape, GenericVector& defVector, int categoryId, int pos) {
            reinterpret_cast<void(*)(void*, ItemInstance&, Recipes::Shape&, GenericVector&,int,int)>(0x0063684c)(ptr1, result, shape, defVector, categoryId, pos);
        }
    };
}