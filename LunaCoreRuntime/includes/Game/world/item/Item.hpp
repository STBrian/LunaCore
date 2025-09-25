#pragma once

#include <string>

#include "Game/game_utils/game_functions.hpp"
#include "Game/game_utils/custom_string.hpp"
#include "Game/world/item/ItemInstance.hpp"

namespace Game {
    class Item {
        public:
        //void* vtable;
        u8 maxStackSize;
        u8 unknown2[3];
        CustomString atlasName;
        int unknown4;
        u16 unknown5;
        u16 itemId;
        CustomString descriptionId;
        CustomString nameId;
        u8 unknown6[6];
        u16 blockID;
        u8 padding[0xac - 0x24];

        class Tier {
            int miningLevel;
            int durability;
            float miningEfficiency;
            int damageBonus;
            int enchantability;

            inline static Tier* WOOD = reinterpret_cast<Tier*>(0x00b0e124);
            inline static Tier* STONE = reinterpret_cast<Tier*>(0x00b0e138);
            inline static Tier* IRON = reinterpret_cast<Tier*>(0x00b0e14c);
            inline static Tier* DIAMOND = reinterpret_cast<Tier*>(0x00b0e160);
            inline static Tier* GOLD = reinterpret_cast<Tier*>(0x00b0e174);
        };

        constexpr static short MAX_ITEMS = 512;
        inline static Item** mItems = reinterpret_cast<Item**>(0x00b0cef0);
        inline static void** renderTable = reinterpret_cast<void**>(0x00b10520);
        inline static ItemInstance** creativeItems = reinterpret_cast<ItemInstance**>(0x00b0d744);
        inline static ItemInstance** creativeItemsEnd = reinterpret_cast<ItemInstance**>(0x00b0d748);

        inline static Item** mItemTable = reinterpret_cast<Item**>(0x00a33f40);
        inline static Item*& mTotem = mItemTable[183];

        inline static void (*registerItems)(void) = reinterpret_cast<void(*)(void)>(0x00563db0);

        /* Item.itemId = itemId + 0x100 */
        Item(const std::string& nameId, short itemId) {
            reinterpret_cast<void*(*)(Item*, CustomString, short)>(0x005790a4)(this, nameId, itemId);
        }

        inline static void addCreativeItem(Item* item, u8 categoryId, s16 position) {
            ItemInstance itemins(item->itemId);
            itemins.unknown1 = categoryId;
            itemins.unknown2 = position;
            reinterpret_cast<void(*)(ItemInstance*)>(0x0056e108)(&itemins);
            reinterpret_cast<void(*)(ItemInstance*)>(0x001d295c)(&itemins);
        }

        virtual void unkFunc1();
        virtual void unkFunc2();
        virtual void setTexture(u32 texnameHash, u32 idx);
    };

    static Item*(*registerItem)(const char* nameId, const short& itemId) = reinterpret_cast<Item*(*)(const char*, const short&)>(0x007cdc8c);
}