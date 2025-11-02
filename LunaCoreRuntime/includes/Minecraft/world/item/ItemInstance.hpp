#pragma once

#include "types.h"

namespace Game {
    class Item;

    class ItemInstance {
        public:
        char field1; // 0x00
        short field2; // 0x02
        char field3; // 0x04
        int field4; // 0x08
        int field5; // 0x0c
        int field6; // 0x10
        short unknown1; // 0x14
        short unknown2; // 0x16
        int field9; // 0x18
        int field10; // 0x1c
        int field11; // 0x20
        int field12; // 0x24
        int field13; // 0x28
        int field14; // 0x2c

        ItemInstance() {
            reinterpret_cast<ItemInstance*(*)(ItemInstance*)>(0x001d28f4)(this);
        }

        ItemInstance(Item* item) {
            reinterpret_cast<ItemInstance*(*)(ItemInstance*, Item*)>(0x001d2444)(this, item);
        }

        ItemInstance(short itemId, short count = 1, short unkn = 0) {
            reinterpret_cast<ItemInstance*(*)(ItemInstance*, short, short, short)>(0x001d2894)(this, itemId, count, unkn);
        }
    };
}