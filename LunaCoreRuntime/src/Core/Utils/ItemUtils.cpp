#include "Core/Utils/ItemUtils.hpp"

#include "game/world/item/CreativeMenu.hpp"

using Item = Minecraft::Item;
using ItemInstance = Minecraft::ItemInstance;

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
        for (const ItemInstance& actual : Minecraft::Creative::InventoryItems) {
            if (actual.mCategory == groupId && actual.mItem->itemId == itemId)
                return actual.mNumber;
        }
        return 0x7FFF;
    }

}