#pragma once

#include "game/asserts.h"
#include "game/gstd/gstd_string.hpp"
#include "game/world/item/Item.hpp"

namespace Minecraft {
    class IronSword : public Item {
        public:
        constexpr static u16 SIZEOF = 0xb4;

        u8 padding[IronSword::SIZEOF - Item::SIZEOF];

        inline static void* VTABLE = (void*)0x998D38;

        IronSword(const gstd::string& nameId, const short itemId) : Item(nameId, itemId) {
            *(void**)this = VTABLE;
        }
    };

    ASSERT_SIZE(IronSword, IronSword::SIZEOF);
}