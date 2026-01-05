#include "Core/Utils/ItemUtils.hpp"

namespace Core::Items {
    Item* SearchItemByName(const std::string& name) {
        short i = 1;
        while (i <= Item::MAX_ITEMS) {
            if (Item::mItems[i] != nullptr) {
                if (Item::mItems[i]->nameId == name)
                    return Item::mItems[i];
            }
            i++;
        }
        return nullptr;
    }

    Item *SearchItemByID(u16 id) {
        if (id <= Item::MAX_ITEMS)
            return Item::mItems[id];
        return nullptr;
    }

    void* GetRenderIDByItemID(u16 id) {
        /*if (id <= Item::MAX_ITEMS) {
            if (Item::renderTable[id] != nullptr) {
                return Item::renderTable[id];
            }
        }*/
        return nullptr;
    }

    u16 GetCreativeItemPositionOfGroup(u16 itemId, u16 groupId) {
        ItemInstance* actualPos = *Item::creativeItems;
        while (actualPos < *Item::creativeItemsEnd) {
            if (actualPos->unknown1 == groupId && (*reinterpret_cast<Item**>((u32)(actualPos) + 0xc))->itemId == itemId)
                return actualPos->unknown2;
            actualPos = actualPos + 1;
        }
        return 0x7FFF;
    }

}