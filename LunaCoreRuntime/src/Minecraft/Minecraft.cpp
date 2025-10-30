//Minecraft class v1.1 by: rairai6895
#include "Minecraft/Minecraft.hpp"
#include <cmath>


void Minecraft::InitBaseAddress() {
    //Player
    Process::Write32(0x5FF184, 0xEB0C65F0);
    Process::Write32(0x91894C, 0xE5940000);
    Process::Write32(0x918950, 0xE58F4000);
    Process::Write32(0x918954, 0xE12FFF1E);
    //Status
    Process::Write32(0x714348, 0xEB081183);
    Process::Write32(0x91895C, 0xED900A13);
    Process::Write32(0x918960, 0xE58F1000);
    Process::Write32(0x918964, 0xE12FFF1E);
    //Level
    Process::Write32(0x21E58C, 0xEB1BE8F6);
    Process::Write32(0x91896C, 0xE2501000);
    Process::Write32(0x918970, 0xE58F3000);
    Process::Write32(0x918974, 0xE12FFF1E);
    //ItemSlot
    Process::Write32(0x677D84, 0xEB0A82FC);
    Process::Write32(0x91897C, 0xE1D000B2);
    Process::Write32(0x918980, 0xE58FC000);
    Process::Write32(0x918984, 0xE12FFF1E);
    //HeldSlot
    Process::Write32(0x3F906C, 0xEB147E46);
    Process::Write32(0x91898C, 0xE5B41008);
    Process::Write32(0x918990, 0xE58F4000);
    Process::Write32(0x918994, 0xE12FFF1E);
    //JumpSneak
    Process::Write32(0x331D0C, 0xEB179B22);
    Process::Write32(0x91899C, 0xE5D0104E);
    Process::Write32(0x9189A0, 0x03510000);
    Process::Write32(0x9189A4, 0x03A01001);
    Process::Write32(0x9189A8, 0x03A01000);
    Process::Write32(0x9189AC, 0x05C0104E);
    Process::Write32(0x9189B0, 0xE58F0004);
    Process::Write32(0x9189B4, 0xE5D0004F);
    Process::Write32(0x9189B8, 0xE12FFF1E);
    //Speed
    Process::Write32(0x197AE4, 0xEB1E03B5);
    Process::Write32(0x9189C0, 0xED900A13);
    Process::Write32(0x9189C4, 0xE58F0000);
    Process::Write32(0x9189C8, 0xE12FFF1E);
    //Sprint
    Process::Write32(0x8B66BC, 0xEB0188C3);
    Process::Write32(0x9189D0, 0xE58F5004);
    Process::Write32(0x9189D4, 0xE1C541D0);
    Process::Write32(0x9189D8, 0xE12FFF1E);
    //GameMode
    Process::Write32(0x735390, 0xEB078D92);
    Process::Write32(0x9189E0, 0xE1A06001);
    Process::Write32(0x9189E4, 0xE58F0000);
    Process::Write32(0x9189E8, 0xE12FFF1E);
    //Weather
    Process::Write32(0x6495F8, 0xEB0B3CFC);
    Process::Write32(0x9189F0, 0xEE000A90);
    Process::Write32(0x9189F4, 0xE58F4000);
    Process::Write32(0x9189F8, 0xE12FFF1E);
}

u32 Minecraft::GetBaseAddress(const Base base) {
    switch (base) {
    case Base::Player:
        return *(u32 *)0x918958;
    case Base::Status:
        return *(u32 *)0x918968;
    case Base::Level:
        return *(u32 *)0x918978;
    case Base::ItemSlot:
        return *(u32 *)0x918988;
    case Base::HeldSlot:
        return *(u32 *)0x918998;
    case Base::JumpSneak:
        return *(u32 *)0x9189BC;
    case Base::Speed:
        return *(u32 *)0x9189CC;
    case Base::Sprint:
        return *(u32 *)0x9189DC;
    case Base::GameMode:
        return *(u32 *)0x9189EC;
    case Base::Weather:
        return *(u32 *)0x9189FC;
    }
    return 0;
}

void Minecraft::PatchProcess() {
    Minecraft::InitBaseAddress();
}


//========================
// InventryUtils
//========================

u32 Minecraft::GetSlotAddress(int slotNum)
{
    const u32 BaseAddr = GetBaseAddress(Base::ItemSlot);
    if (BaseAddr) {
        const u32 SlotOffset = 0x30; // 各スロットの間隔
        const u32 FirstSlotOffset = 0x1B0; // スロット1のオフセット
        const int SlotCount = 36; // スロットの総数
        if (slotNum >= 1 && slotNum <= SlotCount) {
            // 指定したスロットのアドレスを計算
            return BaseAddr + FirstSlotOffset + (slotNum - 1) * SlotOffset;
        } return 0;
    } return 0;
}

u32 Minecraft::GetItemID(int slotNum)
{
    ItemData* const SlotAddr = (ItemData*)GetSlotAddress(slotNum);
    if (SlotAddr) {
        u32 itemID = SlotAddr->itemID;
        return itemID;
    } return 0;
}

void Minecraft::SetItemID(int slotNum, u32 itemID)
{
    ItemData* const SlotAddr = (ItemData*)GetSlotAddress(slotNum);
    if (SlotAddr) {
        SlotAddr->itemID = itemID;
    }
}

int Minecraft::GetItemCount(int slotNum)
{
    ItemData* const SlotAddr = (ItemData*)GetSlotAddress(slotNum);
    if (SlotAddr) {
        int count = SlotAddr->count;
        return count;
    } return 0;
}

void Minecraft::SetItemCount(int slotNum, int count)
{
    ItemData* const SlotAddr = (ItemData*)GetSlotAddress(slotNum);
    if (SlotAddr) {
        SlotAddr->count = count;
    }
}

u16 Minecraft::GetItemData(int slotNum)
{
    ItemData* const SlotAddr = (ItemData*)GetSlotAddress(slotNum);
    if (SlotAddr) {
        u16 dataValue = SlotAddr->dataValue;
        return dataValue;
    } return 0;
}

void Minecraft::SetItemData(int slotNum, u16 dataValue)
{
    ItemData* const SlotAddr = (ItemData*)GetSlotAddress(slotNum);
    if (SlotAddr) {
        SlotAddr->dataValue = dataValue;
    }
}

void Minecraft::SwapSlotItems(int slotA, int slotB)
{
    if (slotA != slotB) {
        const u32 addrA = GetSlotAddress(slotA);
        const u32 addrB = GetSlotAddress(slotB);

        if (addrA && addrB) {
            ItemData dataA, dataB;
            Process::CopyMemory(&dataA, (void*)addrA, sizeof(ItemData));
            Process::CopyMemory(&dataB, (void*)addrB, sizeof(ItemData));
            Process::CopyMemory((void*)addrA, &dataB, sizeof(ItemData));
            Process::CopyMemory((void*)addrB, &dataA, sizeof(ItemData));
        }
    }
}

u32 Minecraft::FindItemAddress(u32 targetItemID, u16 targetDataValue, bool useDataValue)
{
    for (int slot = 0; slot < 37; ++slot) {
        const u32 SlotAddr = GetSlotAddress(slot);
        if (SlotAddr == 0)
            continue;
        u32 itemID = 0;
        u16 dataValue = 0;
        Process::Read32(SlotAddr + 0x0C, itemID);
        Process::Read16(SlotAddr + 0x02, dataValue);
        if (itemID == targetItemID && (!useDataValue || dataValue == targetDataValue))
            return SlotAddr;
    }
    return 0;
}


std::vector<u32> Minecraft::FindAllItemAddresses(u32 targetItemID, u16 targetDataValue, bool useDataValue)
{
    std::vector<u32> addresses;
    for (int slot = 0; slot < 37; ++slot) {
        const u32 SlotAddr = GetSlotAddress(slot);
        if (SlotAddr == 0)
            continue;
        u32 itemID = 0;
        u16 dataValue = 0;
        Process::Read32(SlotAddr + 0x0C, itemID);
        Process::Read16(SlotAddr + 0x02, dataValue);
        if (itemID == targetItemID && (!useDataValue || dataValue == targetDataValue))
            addresses.push_back(SlotAddr);
    }
    return addresses;
}



u32 Minecraft::FindSmallestItemAddress(u32 targetItemID, u16 targetDataValue, bool useDataValue)
{
    std::vector<u32> itemAddresses = FindAllItemAddresses(targetItemID, targetDataValue, useDataValue);
    u32 smallestAddr = 0xFFFFFFFF;
    for (u32 addr : itemAddresses) {
        if (addr < smallestAddr)
            smallestAddr = addr;
    }
    return smallestAddr == 0xFFFFFFFF ? 0 : smallestAddr;
}


int Minecraft::GetSlotNumberFromAddress(u32 address)
{
    const u32 BaseAddr = GetBaseAddress(Base::ItemSlot);
    if (BaseAddr) {
        const u32 FirstSlotOffset = 0x1B0;
        const u32 SlotOffset = 0x30;
        // アドレスがスロットアドレス範囲内にあるかを確認
        if (address >= BaseAddr + FirstSlotOffset &&
            address < BaseAddr + FirstSlotOffset + 36 * SlotOffset) {
            // スロット番号を計算して返す
            return (address - BaseAddr - FirstSlotOffset) / SlotOffset + 1;
        }
        return 0;
    } return 0;
    
}

int Minecraft::GetHeldSlotNumber()
{
    const u32 BaseAddr = GetBaseAddress(Base::HeldSlot);
    static u8 heldSlot;
    if (BaseAddr) {
        Process::Read8(BaseAddr, heldSlot);
        return heldSlot + 1;
    } return -1;
}

u32 Minecraft::GetHeldSlotAddress()
{
    int slotNum = GetHeldSlotNumber();
    return GetSlotAddress(slotNum);
}

void Minecraft::SetHeldSlotNumber(int slotNum)
{
    if (slotNum >= 1 && slotNum <= 9) {
        const u32 BaseAddr1 = GetBaseAddress(Base::HeldSlot);
        static u32 BaseAddr2;
        u8 heldSlot = slotNum - 1;
        Process::Read32(0x0835EAC0, BaseAddr2);
        if (BaseAddr1 && BaseAddr2) {
            Process::Write8(BaseAddr1, heldSlot);
            Process::Write8(BaseAddr2 + 0x225C, heldSlot);
        }
    }
}


int Minecraft::GetCountItemInInventory(u32 targetItemID, u16 targetDataValue, bool useDataValue)
{
    int totalCount = 0;

    for (int slot = 0; slot < 37; ++slot) {
        ItemData* const SlotAddr = (ItemData*)GetSlotAddress(slot);
        if (SlotAddr) {
            u32 ItemID    = SlotAddr->itemID;
            u16 DataValue = SlotAddr->dataValue;
            u8  Count     = SlotAddr->count;
            if (ItemID == targetItemID && (!useDataValue || DataValue == targetDataValue))
                totalCount += Count;
        } continue;
    } return totalCount;
}

// 成功時はスロット番号を返し、失敗時は -1 を返す
int Minecraft::GiveItems(ItemData data)
{
    ItemData* const EmptySlotAddr = (ItemData*)FindSmallestItemAddress(0, 0, false);
    if (EmptySlotAddr) {
        EmptySlotAddr->count = data.count;
        EmptySlotAddr->dataValue = data.dataValue;
        EmptySlotAddr->itemID = data.itemID;
        EmptySlotAddr->renderID = data.renderID;
        return GetSlotNumberFromAddress((u32)EmptySlotAddr);
    } return -1;
}

std::vector<Minecraft::ItemEntry> Minecraft::itemList;
void Minecraft::AddItemEntry(const std::string& name, u32 idAddress, u16 dataValue)
{
    itemList.push_back({name, idAddress, dataValue});
    //OSD::Notify("Item added: " << name << ", ID: " << idAddress << ", DataValue: " << dataValue);
}

void InitItemList()
{
    Minecraft::AddItemEntry("Stone", 0x00B0CEF4, 0);
    Minecraft::AddItemEntry("Granite", 0x00B0CEF4, 1);
    Minecraft::AddItemEntry("Polished Granite", 0x00B0CEF4, 2);
    Minecraft::AddItemEntry("Diorite", 0x00B0CEF4, 3);
    Minecraft::AddItemEntry("Polished Diorite", 0x00B0CEF4, 4);
    Minecraft::AddItemEntry("Andesite", 0x00B0CEF4, 5);
    Minecraft::AddItemEntry("Polished Andesite", 0x00B0CEF4, 5);
    Minecraft::AddItemEntry("Grass Block", 0x00B0CEF8, 0);
    Minecraft::AddItemEntry("Dirt", 0x00B0CEFC, 0);
    Minecraft::AddItemEntry("Coarse Dirt", 0x00B0CEFC, 0);
    Minecraft::AddItemEntry("Cobblestone", 0x00B0CF00, 0);
    Minecraft::AddItemEntry("Oak Wood Planks", 0x00B0CF04, 0);
    Minecraft::AddItemEntry("Spruce Wood Planks", 0x00B0CF04, 1);
    Minecraft::AddItemEntry("Birch Wood Planks", 0x00B0CF04, 2);
    Minecraft::AddItemEntry("Jungle Wood Planks", 0x00B0CF04, 3);
    Minecraft::AddItemEntry("Acacia Wood Planks", 0x00B0CF04, 4);
    Minecraft::AddItemEntry("Dark Oak Wood Planks", 0x00B0CF04, 5);
    Minecraft::AddItemEntry("Oak Sapling", 0x00B0CF08, 0);
    Minecraft::AddItemEntry("Spruce Sapling", 0x00B0CF08, 1);
    Minecraft::AddItemEntry("Birch Sapling", 0x00B0CF08, 2);
    Minecraft::AddItemEntry("Jungle Sapling", 0x00B0CF08, 3);
    Minecraft::AddItemEntry("Acacia Sapling", 0x00B0CF08, 4);
    Minecraft::AddItemEntry("Dark Oak Sapling", 0x00B0CF08, 5);
    Minecraft::AddItemEntry("Bedrock", 0x00B0CF0C, 0);
    Minecraft::AddItemEntry("Flowing Water", 0x00B0CF10, 0);
    Minecraft::AddItemEntry("Still Water", 0x00B0CF14, 0);
    Minecraft::AddItemEntry("Flowing Lava", 0x00B0CF18, 0);
    Minecraft::AddItemEntry("Still Lava", 0x00B0CF1C, 0);
    Minecraft::AddItemEntry("Sand", 0x00B0CF20, 0);
    Minecraft::AddItemEntry("Red Sand", 0x00B0CF20, 1);
    Minecraft::AddItemEntry("Gravel", 0x00B0CF24, 0);
    Minecraft::AddItemEntry("Gold Ore", 0x00B0CF28, 0);
    Minecraft::AddItemEntry("Iron Ore", 0x00B0CF2C, 0);
    Minecraft::AddItemEntry("Coal Ore", 0x00B0CF30, 0);
    Minecraft::AddItemEntry("Oak Wood", 0x00B0CF34, 0);
    Minecraft::AddItemEntry("Spruce Wood", 0x00B0CF34, 1);
    Minecraft::AddItemEntry("Birch Wood", 0x00B0CF34, 2);
    Minecraft::AddItemEntry("Jungle Wood", 0x00B0CF34, 3);
    //Minecraft::AddItemEntry("Acacia Wood", 0x00B0CF34, 0);
    //Minecraft::AddItemEntry("Dark Oak Wood", 0x00B0CF34, 0);
    Minecraft::AddItemEntry("Oak Leaves", 0x00B0CF38, 0);
    Minecraft::AddItemEntry("Spruce Leaves", 0x00B0CF38, 1);
    Minecraft::AddItemEntry("Birch Leaves", 0x00B0CF38, 2);
    Minecraft::AddItemEntry("Jungle Leaves", 0x00B0CF38, 3);
    //Minecraft::AddItemEntry("Acacia Leaves", 0x00B0CF38, 0);
    //Minecraft::AddItemEntry("Dark Oak Leaves", 0x00B0CF38, 0);
    Minecraft::AddItemEntry("Sponge", 0x00B0CF3C, 0);
    Minecraft::AddItemEntry("Wet Sponge", 0x00B0CF3C, 1);
    Minecraft::AddItemEntry("Glass", 0x00B0CF40, 0);
    Minecraft::AddItemEntry("Lapis Lazuli Ore", 0x00B0CF44, 0);
    Minecraft::AddItemEntry("Lapis Lazuli Block", 0x00B0CF48, 0);
    Minecraft::AddItemEntry("Dispenser", 0x00B0CF4C, 0);
    Minecraft::AddItemEntry("Sandstone", 0x00B0CF50, 0);
    Minecraft::AddItemEntry("Chiseled Sandstone", 0x00B0CF50, 1);
    Minecraft::AddItemEntry("Smooth Sandstone", 0x00B0CF50, 2);
    Minecraft::AddItemEntry("Note Block", 0x00B0CF54, 0);
    Minecraft::AddItemEntry("Bed", 0x00B0CF58, 0);
    Minecraft::AddItemEntry("Powered Rail", 0x00B0CF5C, 0);
    Minecraft::AddItemEntry("Detector Rail", 0x00B0CF60, 0);
    Minecraft::AddItemEntry("Sticky Piston", 0x00B0CF64, 0);
    Minecraft::AddItemEntry("Cobweb", 0x00B0CF68, 0);
    Minecraft::AddItemEntry("Fern(0)", 0x00B0CF6C, 0);
    Minecraft::AddItemEntry("Grass", 0x00B0CF6C, 1);
    Minecraft::AddItemEntry("Fern(1)", 0x00B0CF6C, 2);
    Minecraft::AddItemEntry("Dead Bush", 0x00B0CF70, 0);
    Minecraft::AddItemEntry("Piston", 0x00B0CF74, 0);
    Minecraft::AddItemEntry("Piston Head", 0x00B0CF78, 0);
    Minecraft::AddItemEntry("White Wool", 0x00B0CF7C, 0);
    Minecraft::AddItemEntry("Orange Wool", 0x00B0CF7C, 1);
    Minecraft::AddItemEntry("Magenta Wool", 0x00B0CF7C, 2);
    Minecraft::AddItemEntry("Light Blue Wool", 0x00B0CF7C, 3);
    Minecraft::AddItemEntry("Yellow Wool", 0x00B0CF7C, 4);
    Minecraft::AddItemEntry("Lime Wool", 0x00B0CF7C, 5);
    Minecraft::AddItemEntry("Pink Wool", 0x00B0CF7C, 6);
    Minecraft::AddItemEntry("Gray Wool", 0x00B0CF7C, 7);
    Minecraft::AddItemEntry("Light Gray Wool", 0x00B0CF7C, 8);
    Minecraft::AddItemEntry("Cyan Wool", 0x00B0CF7C, 9);
    Minecraft::AddItemEntry("Purple Wool", 0x00B0CF7C, 10);
    Minecraft::AddItemEntry("Blue Wool", 0x00B0CF7C, 11);
    Minecraft::AddItemEntry("Brown Wool", 0x00B0CF7C, 12);
    Minecraft::AddItemEntry("Green Wool", 0x00B0CF7C, 13);
    Minecraft::AddItemEntry("Red Wool", 0x00B0CF7C, 14);
    Minecraft::AddItemEntry("Black Wool", 0x00B0CF7C, 15);
    Minecraft::AddItemEntry("Dandelion(0)", 0x00B0CF84, 0);
    Minecraft::AddItemEntry("Dandelion(1)", 0x00B0CF84, 1);
    Minecraft::AddItemEntry("Poppy", 0x00B0CF88, 0);
    Minecraft::AddItemEntry("Blue Orchid", 0x00B0CF88, 1);
    Minecraft::AddItemEntry("Allium", 0x00B0CF88, 2);
    Minecraft::AddItemEntry("Azure Bluet", 0x00B0CF88, 3);
    Minecraft::AddItemEntry("Red Tulip", 0x00B0CF88, 4);
    Minecraft::AddItemEntry("Orange Tulip", 0x00B0CF88, 5);
    Minecraft::AddItemEntry("White Tulip", 0x00B0CF88, 6);
    Minecraft::AddItemEntry("Pink Tulip", 0x00B0CF88, 7);
    Minecraft::AddItemEntry("Oxeye Daisy", 0x00B0CF88, 8);
    Minecraft::AddItemEntry("Brown Mushroom", 0x00B0CF8C, 0);
    Minecraft::AddItemEntry("Red Mushroom", 0x00B0CF90, 0);
    Minecraft::AddItemEntry("Block of Gold", 0x00B0CF94, 0);
    Minecraft::AddItemEntry("Block of Iron", 0x00B0CF98, 0);
    Minecraft::AddItemEntry("Stone Slab(0)", 0x00B0CF9C, 0);
    Minecraft::AddItemEntry("Sandstone Slab(0)", 0x00B0CF9C, 1);
    Minecraft::AddItemEntry("Wooden Slab(0)", 0x00B0CF9C, 2);
    Minecraft::AddItemEntry("Cobblestone Slab(0)", 0x00B0CF9C, 3);
    Minecraft::AddItemEntry("Bricks Slab(0)", 0x00B0CF9C, 4);
    Minecraft::AddItemEntry("Stone Bricks Slab(0)", 0x00B0CF9C, 5);
    Minecraft::AddItemEntry("Quartz Slab(0)", 0x00B0CF9C, 6);
    Minecraft::AddItemEntry("Nether Brick Slab(0)", 0x00B0CF9C, 7);
    Minecraft::AddItemEntry("Stone Slab(1)", 0x00B0CFA0, 0);
    Minecraft::AddItemEntry("Sandstone Slab(1)", 0x00B0CFA0, 1);
    Minecraft::AddItemEntry("Wooden Slab(1)", 0x00B0CFA0, 2);
    Minecraft::AddItemEntry("Cobblestone Slab(1)", 0x00B0CFA0, 3);
    Minecraft::AddItemEntry("Bricks Slab(1)", 0x00B0CFA0, 4);
    Minecraft::AddItemEntry("Stone Bricks Slab(1)", 0x00B0CFA0, 5);
    Minecraft::AddItemEntry("Quartz Slab(1)", 0x00B0CFA0, 6);
    Minecraft::AddItemEntry("Nether Brick Slab(1)", 0x00B0CFA0, 7);
    Minecraft::AddItemEntry("Stone Slab(2)", 0x00B0CFA0, 8);
    Minecraft::AddItemEntry("Sandstone Slab(2)", 0x00B0CFA0, 9);
    Minecraft::AddItemEntry("Wooden Slab(2)", 0x00B0CFA0, 10);
    Minecraft::AddItemEntry("Cobblestone Slab(2)", 0x00B0CFA0, 11);
    Minecraft::AddItemEntry("Bricks Slab(2)", 0x00B0CFA0, 12);
    Minecraft::AddItemEntry("Stone Bricks Slab(2)", 0x00B0CFA0, 13);
    Minecraft::AddItemEntry("Quartz Slab(2)", 0x00B0CFA0, 14);
    Minecraft::AddItemEntry("Nether Brick Slab(2)", 0x00B0CFA0, 15);
    Minecraft::AddItemEntry("Bricks", 0x00B0CFA4, 0);
    Minecraft::AddItemEntry("TNT", 0x00B0CFA8, 0);
    Minecraft::AddItemEntry("Bookshelf", 0x00B0CFAC, 0);
    Minecraft::AddItemEntry("Moss Stone", 0x00B0CFB0, 0);
    Minecraft::AddItemEntry("Obsidian", 0x00B0CFB4, 0);
    Minecraft::AddItemEntry("Torch", 0x00B0CFB8, 0);
    Minecraft::AddItemEntry("Fire", 0x00B0CFBC, 0);
    Minecraft::AddItemEntry("Monster Spawner", 0x00B0CFC0, 0);

    Minecraft::AddItemEntry("Oak Wood Stairs", 0x00B0CFC4, 0);
    Minecraft::AddItemEntry("Chest", 0x00B0CFC8, 0);
    Minecraft::AddItemEntry("Redstone Dust(0)", 0x00B0CFCC, 0);
    Minecraft::AddItemEntry("Redstone Dust(1)", 0x00B0CFCC, 1);
    Minecraft::AddItemEntry("Redstone Dust(2)", 0x00B0CFCC, 2);
    Minecraft::AddItemEntry("Redstone Dust(3)", 0x00B0CFCC, 3);
    Minecraft::AddItemEntry("Redstone Dust(4)", 0x00B0CFCC, 4);
    Minecraft::AddItemEntry("Redstone Dust(5)", 0x00B0CFCC, 5);
    Minecraft::AddItemEntry("Redstone Dust(6)", 0x00B0CFCC, 6);
    Minecraft::AddItemEntry("Redstone Dust(7)", 0x00B0CFCC, 7);
    Minecraft::AddItemEntry("Diamond Ore", 0x00B0CFD0, 0);
    Minecraft::AddItemEntry("Block of Diamond", 0x00B0CFD4, 0);
    Minecraft::AddItemEntry("Crafting Table", 0x00B0CFD8, 0);
    Minecraft::AddItemEntry("Crops(0)", 0x00B0CFDC, 0);
    Minecraft::AddItemEntry("Crops(1)", 0x00B0CFDC, 1);
    Minecraft::AddItemEntry("Crops(2)", 0x00B0CFDC, 2);
    Minecraft::AddItemEntry("Crops(3)", 0x00B0CFDC, 3);
    Minecraft::AddItemEntry("Crops(4)", 0x00B0CFDC, 4);
    Minecraft::AddItemEntry("Crops(5)", 0x00B0CFDC, 5);
    Minecraft::AddItemEntry("Crops(6)", 0x00B0CFDC, 6);
    Minecraft::AddItemEntry("Crops(7)", 0x00B0CFDC, 7);
    Minecraft::AddItemEntry("Farmland(0)", 0x00B0CFE0, 0);
    Minecraft::AddItemEntry("Farmland(1)", 0x00B0CFE0, 1);
    Minecraft::AddItemEntry("Furnace", 0x00B0CFE4, 0);
    Minecraft::AddItemEntry("Burning Furnace", 0x00B0CFE8, 0);
    Minecraft::AddItemEntry("Standing Sign Block", 0x00B0CFEC, 0);
    Minecraft::AddItemEntry("Oak Door Block", 0x00B0CFF0, 0);
    Minecraft::AddItemEntry("Ladder", 0x00B0CFF4, 0);
    Minecraft::AddItemEntry("Rail", 0x00B0CFF8, 0);
    Minecraft::AddItemEntry("Cobblestone Stairs", 0x00B0CFFC, 0);
    Minecraft::AddItemEntry("Wall-mounted Sign Block", 0x00B0D000, 0);
    Minecraft::AddItemEntry("Lever", 0x00B0D004, 0);
    Minecraft::AddItemEntry("Stone Pressure Plate", 0x00B0D008, 0);
    Minecraft::AddItemEntry("Iron Door Block", 0x00B0D00C, 0);
    Minecraft::AddItemEntry("Wooden Pressure Plate", 0x00B0D010, 0);
    Minecraft::AddItemEntry("Redstone Ore", 0x00B0D014, 0);
    Minecraft::AddItemEntry("Glowing Redstone Ore", 0x00B0D018, 0);
    Minecraft::AddItemEntry("Redstone Torch(off)", 0x00B0D01C, 0);
    Minecraft::AddItemEntry("Redstone Torch(on)", 0x00B0D020, 0);
    Minecraft::AddItemEntry("Stone Button(0)", 0x00B0D024, 0);
    Minecraft::AddItemEntry("Stone Button(1)", 0x00B0D024, 1);
    Minecraft::AddItemEntry("Stone Button(2)", 0x00B0D024, 2);
    Minecraft::AddItemEntry("Stone Button(3)", 0x00B0D024, 3);
    Minecraft::AddItemEntry("Stone Button(4)", 0x00B0D024, 4);
    Minecraft::AddItemEntry("Stone Button(5)", 0x00B0D024, 5);
    Minecraft::AddItemEntry("Top Snow(0)", 0x00B0D028, 0);
    Minecraft::AddItemEntry("Top Snow(1)", 0x00B0D028, 1);
    Minecraft::AddItemEntry("Top Snow(2)", 0x00B0D028, 2);
    Minecraft::AddItemEntry("Top Snow(3)", 0x00B0D028, 3);
    Minecraft::AddItemEntry("Top Snow(4)", 0x00B0D028, 4);
    Minecraft::AddItemEntry("Top Snow(5)", 0x00B0D028, 5);
    Minecraft::AddItemEntry("Top Snow(6)", 0x00B0D028, 6);
    Minecraft::AddItemEntry("Top Snow(7)", 0x00B0D028, 7);
    Minecraft::AddItemEntry("Ice", 0x00B0D02C, 0);
    Minecraft::AddItemEntry("Snow", 0x00B0D030, 0);
    Minecraft::AddItemEntry("Cactus", 0x00B0D034, 0);
    Minecraft::AddItemEntry("Clay", 0x00B0D038, 0);
    Minecraft::AddItemEntry("Sugar cane", 0x00B0D03C, 0);
    Minecraft::AddItemEntry("Oak Fence", 0x00B0D044, 0);
    Minecraft::AddItemEntry("Spruce Fence", 0x00B0D044, 1);
    Minecraft::AddItemEntry("Birch Fence", 0x00B0D044, 2);
    Minecraft::AddItemEntry("Jungle Fence", 0x00B0D044, 3);
    Minecraft::AddItemEntry("Acacia Fence", 0x00B0D044, 4);
    Minecraft::AddItemEntry("Dark Oak Fence", 0x00B0D044, 5);
    Minecraft::AddItemEntry("Pumpkin", 0x00B0D048, 0);
    Minecraft::AddItemEntry("Netherrack", 0x00B0D04C, 0);
    Minecraft::AddItemEntry("Soul Sand", 0x00B0D050, 0);
    Minecraft::AddItemEntry("Glowstone", 0x00B0D054, 0);
    Minecraft::AddItemEntry("Nether Portal", 0x00B0D058, 0);
    Minecraft::AddItemEntry("Jack o'Lantern", 0x00B0D05C, 0);
    Minecraft::AddItemEntry("Cake Block(0)", 0x00B0D060, 0);
    Minecraft::AddItemEntry("Cake Block(1)", 0x00B0D060, 1);
    Minecraft::AddItemEntry("Cake Block(2)", 0x00B0D060, 2);
    Minecraft::AddItemEntry("Cake Block(3)", 0x00B0D060, 3);
    Minecraft::AddItemEntry("Cake Block(4)", 0x00B0D060, 4);
    Minecraft::AddItemEntry("Cake Block(5)", 0x00B0D060, 5);
    Minecraft::AddItemEntry("Cake Block(6)", 0x00B0D060, 6);
    Minecraft::AddItemEntry("Cake Block(7)", 0x00B0D060, 7);
    Minecraft::AddItemEntry("Redstone Repeater Block(off)", 0x00B0D064, 0);
    Minecraft::AddItemEntry("Redstone Repeater Block(on)", 0x00B0D068, 0);
    Minecraft::AddItemEntry("InvisibleBedrock", 0x00B0D06C, 0);
    Minecraft::AddItemEntry("Wooden Trapdoor(0)", 0x00B0D070, 0);
    Minecraft::AddItemEntry("Wooden Trapdoor(1)", 0x00B0D070, 4);
    Minecraft::AddItemEntry("Wooden Trapdoor(2)", 0x00B0D070, 8);
    Minecraft::AddItemEntry("Wooden Trapdoor(3)", 0x00B0D070, 9);
    Minecraft::AddItemEntry("Wooden Trapdoor(4)", 0x00B0D070, 10);
    Minecraft::AddItemEntry("Stone Monster Egg", 0x00B0D074, 0);
    Minecraft::AddItemEntry("Cobblestone Monster Egg", 0x00B0D074, 1);
    Minecraft::AddItemEntry("Stone Brick Monster Egg", 0x00B0D074, 2);
    Minecraft::AddItemEntry("Mossy Stone Brick Monster Egg", 0x00B0D074, 3);
    Minecraft::AddItemEntry("Cracked Stone Brick Monster Egg", 0x00B0D074, 4);
    Minecraft::AddItemEntry("Chiseled Stone Brick Monster Egg", 0x00B0D074, 5);
    Minecraft::AddItemEntry("Stone Bricks", 0x00B0D078, 0);
    Minecraft::AddItemEntry("Mossy Stone Bricks", 0x00B0D078, 1);
    Minecraft::AddItemEntry("Cracked Stone Bricks", 0x00B0D078, 2);
    Minecraft::AddItemEntry("Chiseled Stone Bricks", 0x00B0D078, 3);
    Minecraft::AddItemEntry("Stone Bricks(smooth)", 0x00B0D078, 4);
    Minecraft::AddItemEntry("Mushroom(0)", 0x00B0D07C, 0);
    Minecraft::AddItemEntry("Mushroom(1)", 0x00B0D07C, 1);
    Minecraft::AddItemEntry("Mushroom(2)", 0x00B0D07C, 2);
    Minecraft::AddItemEntry("Mushroom(3)", 0x00B0D07C, 3);
    Minecraft::AddItemEntry("Mushroom(4)", 0x00B0D07C, 4);
    Minecraft::AddItemEntry("Mushroom(5)", 0x00B0D07C, 5);
    Minecraft::AddItemEntry("Mushroom(6)", 0x00B0D07C, 6);
    Minecraft::AddItemEntry("Mushroom(7)", 0x00B0D07C, 7);
    Minecraft::AddItemEntry("Mushroom(8)", 0x00B0D07C, 8);
    Minecraft::AddItemEntry("Mushroom(9)", 0x00B0D07C, 9);
    Minecraft::AddItemEntry("Mushroom(10)", 0x00B0D07C, 10);
    Minecraft::AddItemEntry("Mushroom(11)", 0x00B0D07C, 11);
    Minecraft::AddItemEntry("Mushroom(12)", 0x00B0D07C, 12);
    Minecraft::AddItemEntry("Mushroom(13)", 0x00B0D07C, 13);
    Minecraft::AddItemEntry("Mushroom(14)", 0x00B0D07C, 14);
    Minecraft::AddItemEntry("Mushroom(15)", 0x00B0D07C, 15);
    Minecraft::AddItemEntry("Mushroom(16)", 0x00B0D080, 0);
    Minecraft::AddItemEntry("Mushroom(17)", 0x00B0D080, 1);
    Minecraft::AddItemEntry("Mushroom(18)", 0x00B0D080, 2);
    Minecraft::AddItemEntry("Mushroom(19)", 0x00B0D080, 3);
    Minecraft::AddItemEntry("Mushroom(20)", 0x00B0D080, 4);
    Minecraft::AddItemEntry("Mushroom(21)", 0x00B0D080, 5);
    Minecraft::AddItemEntry("Mushroom(22)", 0x00B0D080, 6);
    Minecraft::AddItemEntry("Mushroom(23)", 0x00B0D080, 7);
    Minecraft::AddItemEntry("Mushroom(24)", 0x00B0D080, 8);
    Minecraft::AddItemEntry("Mushroom(25)", 0x00B0D080, 9);
    Minecraft::AddItemEntry("Mushroom(26)", 0x00B0D080, 10);
    Minecraft::AddItemEntry("Mushroom(27)", 0x00B0D080, 11);
    Minecraft::AddItemEntry("Mushroom(28)", 0x00B0D080, 12);
    Minecraft::AddItemEntry("Mushroom(29)", 0x00B0D080, 13);
    Minecraft::AddItemEntry("Mushroom(30)", 0x00B0D080, 14);
    Minecraft::AddItemEntry("Mushroom(31)", 0x00B0D080, 15);
    Minecraft::AddItemEntry("Iron Bars", 0x00B0D084, 0);
    Minecraft::AddItemEntry("Glass Pane", 0x00B0D088, 0);
    Minecraft::AddItemEntry("Melon Block", 0x00B0D08C, 0);
    Minecraft::AddItemEntry("Pumpkin Stem(0)", 0x00B0D090, 0);
    Minecraft::AddItemEntry("Pumpkin Stem(1)", 0x00B0D090, 1);
    Minecraft::AddItemEntry("Pumpkin Stem(2)", 0x00B0D090, 2);
    Minecraft::AddItemEntry("Pumpkin Stem(3)", 0x00B0D090, 3);
    Minecraft::AddItemEntry("Pumpkin Stem(4)", 0x00B0D090, 4);
    Minecraft::AddItemEntry("Pumpkin Stem(5)", 0x00B0D090, 5);
    Minecraft::AddItemEntry("Pumpkin Stem(6)", 0x00B0D090, 6);
    Minecraft::AddItemEntry("Pumpkin Stem(7)", 0x00B0D090, 7);
    Minecraft::AddItemEntry("Melon Stem(0)", 0x00B0D094, 0);
    Minecraft::AddItemEntry("Melon Stem(1)", 0x00B0D094, 1);
    Minecraft::AddItemEntry("Melon Stem(2)", 0x00B0D094, 2);
    Minecraft::AddItemEntry("Melon Stem(3)", 0x00B0D094, 3);
    Minecraft::AddItemEntry("Melon Stem(4)", 0x00B0D094, 4);
    Minecraft::AddItemEntry("Melon Stem(5)", 0x00B0D094, 5);
    Minecraft::AddItemEntry("Melon Stem(6)", 0x00B0D094, 6);
    Minecraft::AddItemEntry("Melon Stem(7)", 0x00B0D094, 7);
    Minecraft::AddItemEntry("Vine", 0x00B0D098, 0);
    Minecraft::AddItemEntry("Oak Fence Gate", 0x00B0D09C, 0);
    Minecraft::AddItemEntry("Brick Stairs", 0x00B0D0A0, 0);
    Minecraft::AddItemEntry("Stone Brick Stairs", 0x00B0D0A4, 0);
    Minecraft::AddItemEntry("Mycelium", 0x00B0D0A8, 0);
    Minecraft::AddItemEntry("Lily Pad", 0x00B0D0AC, 0);
    Minecraft::AddItemEntry("Nether Brick", 0x00B0D0B0, 0);
    Minecraft::AddItemEntry("Nether Brick Fence", 0x00B0D0B4, 0);
    Minecraft::AddItemEntry("Nether Brick Stairs", 0x00B0D0B8, 0);
    Minecraft::AddItemEntry("Nether Wart(0)", 0x00B0D0BC, 0);
    Minecraft::AddItemEntry("Nether Wart(1)", 0x00B0D0BC, 1);
    Minecraft::AddItemEntry("Nether Wart(2)", 0x00B0D0BC, 2);
    Minecraft::AddItemEntry("Nether Wart(3)", 0x00B0D0BC, 3);
    Minecraft::AddItemEntry("Enchantment Table", 0x00B0D0C0, 0);
    Minecraft::AddItemEntry("Brewing Stand", 0x00B0D0C4, 0);
    Minecraft::AddItemEntry("Cauldron", 0x00B0D0C8, 0);
    Minecraft::AddItemEntry("End portal", 0x00B0D0CC, 0);
    Minecraft::AddItemEntry("End portal Frame(0)", 0x00B0D0D0, 0);
    Minecraft::AddItemEntry("End portal Frame(1)", 0x00B0D0D0, 1);
    Minecraft::AddItemEntry("End portal Frame(2)", 0x00B0D0D0, 2);
    Minecraft::AddItemEntry("End portal Frame(3)", 0x00B0D0D0, 3);
    Minecraft::AddItemEntry("End portal Frame(4)", 0x00B0D0D0, 4);
    Minecraft::AddItemEntry("End portal Frame(5)", 0x00B0D0D0, 5);
    Minecraft::AddItemEntry("End portal Frame(6)", 0x00B0D0D0, 6);
    Minecraft::AddItemEntry("End portal Frame(7)", 0x00B0D0D0, 7);
    Minecraft::AddItemEntry("End Stone", 0x00B0D0D4, 0);
    Minecraft::AddItemEntry("Dragon Egg", 0x00B0D0D8, 0);
    Minecraft::AddItemEntry("Redstone Lamp(inactive)", 0x00B0D0DC, 0);
    Minecraft::AddItemEntry("Redstone Lamp(active)", 0x00B0D0E0, 0);
    Minecraft::AddItemEntry("Dropper", 0x00B0D0E4, 0);
    Minecraft::AddItemEntry("Activator Rail", 0x00B0D0E8, 0);
    Minecraft::AddItemEntry("Cocoa(0)", 0x00B0D0EC, 0);
    Minecraft::AddItemEntry("Cocoa(1)", 0x00B0D0EC, 1);
    Minecraft::AddItemEntry("Cocoa(2)", 0x00B0D0EC, 2);
    Minecraft::AddItemEntry("Sandstone Stairs", 0x00B0D0F0, 0);
    Minecraft::AddItemEntry("Emerald Ore", 0x00B0D0F4, 0);
    Minecraft::AddItemEntry("Ender Chest", 0x00B0D0F8, 0);
    Minecraft::AddItemEntry("Tripwire Hook", 0x00B0D0FC, 0);
    Minecraft::AddItemEntry("Tripwire", 0x00B0D100, 0);
    Minecraft::AddItemEntry("Block of Emerald", 0x00B0D104, 0);
    Minecraft::AddItemEntry("Spruce Wood Stairs", 0x00B0D108, 0);
    Minecraft::AddItemEntry("Birch Wood Stairs", 0x00B0D10C, 0);
    Minecraft::AddItemEntry("Jungle Wood Stairs", 0x00B0D110, 0);
    Minecraft::AddItemEntry("Command Block", 0x00B0D114, 0);
    Minecraft::AddItemEntry("Beacon", 0x00B0D118, 0);
    Minecraft::AddItemEntry("Cobblestone Wall", 0x00B0D11C, 0);
    Minecraft::AddItemEntry("Mossy Cobblestone Wall", 0x00B0D11C, 1);
    Minecraft::AddItemEntry("Flower Pot", 0x00B0D120, 0);
    Minecraft::AddItemEntry("Carrots(0)", 0x00B0D124, 0);
    Minecraft::AddItemEntry("Carrots(1)", 0x00B0D124, 1);
    Minecraft::AddItemEntry("Carrots(2)", 0x00B0D124, 2);
    Minecraft::AddItemEntry("Carrots(3)", 0x00B0D124, 3);
    Minecraft::AddItemEntry("Carrots(4)", 0x00B0D124, 4);
    Minecraft::AddItemEntry("Carrots(5)", 0x00B0D124, 5);
    Minecraft::AddItemEntry("Carrots(6)", 0x00B0D124, 6);
    Minecraft::AddItemEntry("Carrots(7)", 0x00B0D124, 7);
    Minecraft::AddItemEntry("Potatoes(0)", 0x00B0D128, 0);
    Minecraft::AddItemEntry("Potatoes(1)", 0x00B0D128, 1);
    Minecraft::AddItemEntry("Potatoes(2)", 0x00B0D128, 2);
    Minecraft::AddItemEntry("Potatoes(3)", 0x00B0D128, 3);
    Minecraft::AddItemEntry("Potatoes(4)", 0x00B0D128, 4);
    Minecraft::AddItemEntry("Potatoes(5)", 0x00B0D128, 5);
    Minecraft::AddItemEntry("Potatoes(6)", 0x00B0D128, 6);
    Minecraft::AddItemEntry("Potatoes(7)", 0x00B0D128, 7);
    Minecraft::AddItemEntry("Wooden Button(0)", 0x00B0D12C, 0);
    Minecraft::AddItemEntry("Wooden Button(1)", 0x00B0D12C, 1);
    Minecraft::AddItemEntry("Wooden Button(2)", 0x00B0D12C, 2);
    Minecraft::AddItemEntry("Wooden Button(3)", 0x00B0D12C, 3);
    Minecraft::AddItemEntry("Wooden Button(4)", 0x00B0D12C, 4);
    Minecraft::AddItemEntry("Wooden Button(5)", 0x00B0D12C, 5);
    Minecraft::AddItemEntry("Mob Head", 0x00B0D130, 0);
    Minecraft::AddItemEntry("Anvil(0)", 0x00B0D134, 0);
    Minecraft::AddItemEntry("Anvil(1)", 0x00B0D134, 1);
    Minecraft::AddItemEntry("Anvil(2)", 0x00B0D134, 2);
    Minecraft::AddItemEntry("Trapped Chest", 0x00B0D138, 0);
    Minecraft::AddItemEntry("Weighted Pressure Plate(light)", 0x00B0D13C, 0);
    Minecraft::AddItemEntry("Weighted Pressure Plate(heavy)", 0x00B0D140, 0);
    Minecraft::AddItemEntry("Redstone Comparator(inactive)", 0x00B0D144, 0);
    Minecraft::AddItemEntry("Redstone Comparator(active)", 0x00B0D148, 0);
    Minecraft::AddItemEntry("Daylight Sensor", 0x00B0D14C, 0);
    Minecraft::AddItemEntry("Block of Redstone", 0x00B0D150, 0);
    Minecraft::AddItemEntry("Nether Quartz Ore", 0x00B0D154, 0);


    // itemList の内容を表示して確認
    //for (const auto& item : Minecraft::itemList) {
    //    OSD::Notify("Item in list: " << item.name);
    //}
    
}

const std::vector<Minecraft::ItemEntry>& Minecraft::GetItemList()
{
    static bool initialized = false;
    if (!initialized)
    {
        InitItemList();
        initialized = true;
        //OSD::Notify("InitItemList(); loaded!");  
    }
    return itemList;
}

std::vector<Minecraft::ItemEntry*> Minecraft::FindItemByName(const std::string& partialName)
{
    std::vector<ItemEntry*> results;

    std::string lowerPartialName = partialName;
    std::transform(lowerPartialName.begin(), lowerPartialName.end(), lowerPartialName.begin(), ::tolower);
    //OSD::Notify("Searching for: " << lowerPartialName);

    for (auto& item : itemList) {
        std::string lowerItemName = item.name;
        std::transform(lowerItemName.begin(), lowerItemName.end(), lowerItemName.begin(), ::tolower);

        if (lowerItemName.find(lowerPartialName) != std::string::npos) {
            results.push_back(&item);
            //OSD::Notify("Found item: " << item.name);
        }
    }

    if (results.empty()) {
        //OSD::Notify("No items found for: " << lowerPartialName);
    }

    return results;
}

std::optional<Minecraft::SelectedItemInfo> Minecraft::ShowItemSelector(
    const std::string& searchTitle,
    const std::string& resultTitle)
{
    std::string SearchWord;
    Keyboard Words(searchTitle);
    Words.Open(SearchWord);

    const auto& itemList = GetItemList();
    std::vector<ItemEntry*> results = FindItemByName(SearchWord);

    std::vector<std::string> namelist;
    for (ItemEntry* item : results) {
        if (item != nullptr)
            namelist.push_back(item->name);
    }

    if (namelist.empty()) {
        return std::nullopt;
    }

    Keyboard Searchedlists(resultTitle, namelist);
    int selectedIndex = Searchedlists.Open();

    if (selectedIndex >= 0 && selectedIndex < static_cast<int>(namelist.size())) {
        ItemEntry* selectedItem = results[selectedIndex];
        u32 itemID;
        Process::Read32(selectedItem->idAddress, itemID);
        u32 renderID = GetRenderID(selectedItem->idAddress);

        SelectedItemInfo info {
            selectedItem->name,
            selectedItem->idAddress,
            itemID,
            selectedItem->dataValue,
            renderID
        };
        return info;
    }
    return std::nullopt;
}

std::optional<Minecraft::SelectedItemInfo> Minecraft::ShowItemSelectorWithPaging(
    const std::string& searchTitle,
    const std::string& resultTitle,
    const std::string& pagingNextTitle,
    const std::string& pagingBackTitle)
{
    std::string SearchWord;
    Keyboard Words(searchTitle);
    Words.Open(SearchWord);

    const auto& itemList = GetItemList();
    std::vector<ItemEntry*> results = FindItemByName(SearchWord);

    if (results.empty())
        return std::nullopt;

    const int itemsPerPage = 50;
    int currentPage = 0;
    int totalPages = (results.size() + itemsPerPage - 1) / itemsPerPage;

    while (true) {
        std::vector<std::string> displayList;
        int startIndex = currentPage * itemsPerPage;
        int endIndex = std::min(startIndex + itemsPerPage, (int)results.size());

        for (int i = startIndex; i < endIndex; ++i) {
            if (results[i])
                displayList.push_back(results[i]->name);
        }

        bool hasPrev = currentPage > 0;
        bool hasNext = currentPage < totalPages - 1;

        if (hasPrev)
            displayList.push_back(pagingBackTitle);
        if (hasNext)
            displayList.push_back(pagingNextTitle);

        Keyboard pageSelector(resultTitle + " (" + std::to_string(currentPage + 1) + "/" + std::to_string(totalPages) + ")", displayList);
        int selectedIndex = pageSelector.Open();

        if (selectedIndex == -1 || selectedIndex == -2)
            return std::nullopt;

        if (hasPrev && selectedIndex == displayList.size() - (hasNext ? 2 : 1)) {
            currentPage--;
            continue;
        }
        if (hasNext && selectedIndex == displayList.size() - 1) {
            currentPage++;
            continue;
        }

        int realIndex = currentPage * itemsPerPage + selectedIndex;
        if (realIndex >= 0 && realIndex < (int)results.size()) {
            ItemEntry* selectedItem = results[realIndex];
            u32 itemID;
            Process::Read32(selectedItem->idAddress, itemID);
            u32 renderID = GetRenderID(selectedItem->idAddress);

            SelectedItemInfo info {
                selectedItem->name,
                selectedItem->idAddress,
                itemID,
                selectedItem->dataValue,
                renderID
            };
            return info;
        }
    }
}

u32 Minecraft::GetRenderID(u32 idAddress)
{
    u32 renderID = 0;
    Process::Read32(idAddress + 0x3630, renderID);
    if (renderID < 0x30000000 || renderID > 0x40000000)
        return 0;
    return renderID;
}

u32 Minecraft::GetArmorSlotAddress(int slotNum)
{
    const u32 BaseAddr = GetBaseAddress(Base::Player);
    const u32 SlotOffset = 0x30; // 防具スロットの間隔
    const u32 FirstSlotOffset = 0xF40;  // 防具スロット1（頭）のオフセット
    const int SlotCount = 4;  // 防具スロットの総数（頭、胸当て、レギンス、ブーツ）
    if (BaseAddr) {
        if (slotNum >= 1 && slotNum <= SlotCount) {
            return BaseAddr + FirstSlotOffset + (slotNum - 1) * SlotOffset;
        } return 0;
    } return 0;
}

u32 Minecraft::GetArmorItemID(int slotNum)
{
    ItemData* const armorSlotAddress = (ItemData*)GetArmorSlotAddress(slotNum);
    static u32 itemID;
    if (armorSlotAddress) {
        itemID = armorSlotAddress->itemID;
        return itemID;
    } return 0;
}

void Minecraft::SetArmorItemID(int slotNum, u32 itemID)
{
    ItemData* const armorSlotAddress = (ItemData*)GetArmorSlotAddress(slotNum);
    if (armorSlotAddress) {
        armorSlotAddress->itemID = itemID;
    }
}

int Minecraft::GetArmorItemCount(int slotNum)
{
    ItemData* const armorSlotAddress = (ItemData*)GetArmorSlotAddress(slotNum);
    static int count;
    if (armorSlotAddress) {
        count = armorSlotAddress->count;
        return count;
    } return 0;
}

void Minecraft::SetArmorItemCount(int slotNum, int count)
{
    ItemData* const armorSlotAddress = (ItemData*)GetArmorSlotAddress(slotNum);
    if (armorSlotAddress) {
        armorSlotAddress->dataValue = count;
    }
}

u16 Minecraft::GetArmorItemData(int slotNum)
{
    ItemData* const armorSlotAddress = (ItemData*)GetArmorSlotAddress(slotNum);
    static u16 dataValue;
    if (armorSlotAddress) {
        dataValue = armorSlotAddress->dataValue;
        return dataValue;
    } return 0;
}

void Minecraft::SetArmorItemData(int slotNum, u16 dataValue)
{
    ItemData* const armorSlotAddress = (ItemData*)GetArmorSlotAddress(slotNum);
    if (armorSlotAddress) {
        armorSlotAddress->dataValue = dataValue;
    }
}



void Minecraft::SwapArmorAndInventoryItem(int armorSlot, int inventorySlot)
{
    const u32 addrArmor = GetArmorSlotAddress(armorSlot);
    const u32 addrInv = GetSlotAddress(inventorySlot);
    ItemData armorData, invData;
    if (addrArmor && addrInv) {
        Process::CopyMemory(&armorData, (void*)addrArmor, sizeof(ItemData));
        Process::CopyMemory(&invData, (void*)addrInv, sizeof(ItemData));
        Process::CopyMemory((void*)addrArmor, &invData, sizeof(ItemData));
        Process::CopyMemory((void*)addrInv, &armorData, sizeof(ItemData));
    }
}


//========================
// PlayerUtils
//========================

float Minecraft::GetCurrentHP()
{
    const u32 BaseAddr = GetBaseAddress(Base::Status);
    static float value;
    if (BaseAddr) {
        Process::ReadFloat(BaseAddr + 0x60, value);
        return value;
    } return -1.0f;
}

float Minecraft::GetMaxHP()
{
    const u32 BaseAddr = GetBaseAddress(Base::Status);
    static float value;
    if (BaseAddr) {
        Process::ReadFloat(BaseAddr + 0x5C, value);
        return value;
    } return -1.0f;
}

void Minecraft::SetCurrentHP(float value)
{
    const u32 BaseAddr = GetBaseAddress(Base::Status);
    if (BaseAddr) {
        Process::WriteFloat(BaseAddr + 0x60, value);
    }
}

void Minecraft::SetMaxHP(float value)
{
    const u32 BaseAddr = GetBaseAddress(Base::Status);
    if (BaseAddr) {
        Process::WriteFloat(BaseAddr + 0x5C, value);
    }
}

float Minecraft::GetCurrentHunger()
{
    const u32 BaseAddr = GetBaseAddress(Base::Status);
    static float value;
    if (BaseAddr) {
        Process::ReadFloat(BaseAddr + 0x6E8, value);
        return value;
    } return -1.0f;
}

float Minecraft::GetMaxHunger()
{
    const u32 BaseAddr = GetBaseAddress(Base::Status);
    static float value;
    if (BaseAddr) {
        Process::ReadFloat(BaseAddr + 0x6E4, value);
        return value;
    } return -1.0f;
}

void Minecraft::SetCurrentHunger(float value)
{
    const u32 BaseAddr = GetBaseAddress(Base::Status);
    if (BaseAddr) {
        Process::WriteFloat(BaseAddr + 0x6E8, value);
    }
}

void Minecraft::SetMaxHunger(float value)
{
    const u32 BaseAddr = GetBaseAddress(Base::Status);
    if (BaseAddr) {
        Process::WriteFloat(BaseAddr + 0x6E4, value);
    }
}

float Minecraft::GetCurrentLevel()
{
    const u32 BaseAddr = GetBaseAddress(Base::Level);
    static float value;
    if (BaseAddr) {
        Process::ReadFloat(BaseAddr + 0x60, value);
        return value;
    } return -1.0f;
}

void Minecraft::SetCurrentLevel(float newLevel)
{
    const u32 BaseAddr = GetBaseAddress(Base::Level);
    if (BaseAddr) {
        Process::WriteFloat(BaseAddr + 0x60, newLevel);
    }
}

float Minecraft::GetXPBarProgress()
{
    const u32 BaseAddr = GetBaseAddress(Base::Level);
    static float value;
    if (BaseAddr) {
        Process::ReadFloat(BaseAddr + 0xC4, value);
        return value;
    } return -1.0f;
}

void Minecraft::SetXPBarProgress(float progress)
{
    const u32 BaseAddr = GetBaseAddress(Base::Level);
    if (BaseAddr) {
        Process::WriteFloat(BaseAddr + 0xC4, progress);
    }
}

void Minecraft::GetPlayerPosition(float& posX, float& posY, float& posZ)
{
    const u32 BaseAddr = GetBaseAddress(Base::Player);
    static float x, y, z;
    if (BaseAddr) {
        Process::ReadFloat(BaseAddr + 0x1C0, x);
        Process::ReadFloat(BaseAddr + 0x1C4, y);
        Process::ReadFloat(BaseAddr + 0x1C8, z);
        posX = std::floor(x);
        posY = std::floor(y);
        posZ = std::floor(z);
    }
}

void Minecraft::SetPlayerPosition(float posX, float posY, float posZ)
{
    const u32 BaseAddr = GetBaseAddress(Base::Player);
    if (BaseAddr) {
        // X座標
        Process::WriteFloat(BaseAddr + 0x288, posX + 0.2f);
        Process::WriteFloat(BaseAddr + 0x294, posX + 0.8f);
        Process::WriteFloat(BaseAddr + 0x1C0, posX + 0.5f);
        // Y座標
        Process::WriteFloat(BaseAddr + 0x28C, posY + 0.0f);
        Process::WriteFloat(BaseAddr + 0x298, posY + 1.8f);
        Process::WriteFloat(BaseAddr + 0x1C4, posY + 1.62f);
        // Z座標
        Process::WriteFloat(BaseAddr + 0x290, posZ + 0.2f);
        Process::WriteFloat(BaseAddr + 0x29C, posZ + 0.8f);
        Process::WriteFloat(BaseAddr + 0x1C8, posZ + 0.5f);
    }
}

bool Minecraft::IsPlayerOnGround()
{
    const u32 BaseAddr = GetBaseAddress(Base::Player);
    static u32 value1;
    static u16 value2;
    if (BaseAddr) {
        Process::Read32(BaseAddr + 0x258, value1);
        Process::Read16(BaseAddr + 0x25D, value2);
        return value1 == 0x01010000 && value2 == 0x0101;
    } return false;
    
}

void Minecraft::SetPlayerOnGround(bool onGround)
{
    const u32 BaseAddr = GetBaseAddress(Base::Player);
    if (BaseAddr) {
        Process::Write32(BaseAddr + 0x258, onGround ? 0x01010000 : 0);
        Process::Write16(BaseAddr + 0x25D, onGround ? 0x0101 : 0);
    }
}

bool Minecraft::IsPlayerSneaking()
{
    const u32 BaseAddr = GetBaseAddress(Base::JumpSneak);
    static u8 value;
    if (BaseAddr) {
        Process::Read8(BaseAddr + 0x4E, value);
        return value == 1;
    } return false;
}

void Minecraft::SetPlayerSneaking(bool enable)
{
    const u32 BaseAddr = GetBaseAddress(Base::JumpSneak);
    if (BaseAddr) {
        Process::Write8(BaseAddr + 0x4E, enable ? 1 : 0);
    }
}

bool Minecraft::IsPlayerJumping()
{
    const u32 BaseAddr = GetBaseAddress(Base::JumpSneak);
    static u8 value;
    if (BaseAddr) {
        Process::Read8(BaseAddr + 0x4D, value);
        return value == 1;
    } return false;
}

void Minecraft::SetPlayerJump(bool enable)
{
    const u32 BaseAddr = GetBaseAddress(Base::JumpSneak);
    if (BaseAddr) {
        Process::Write8(BaseAddr + 0x4D, enable ? 1 : 0);
    }
}

float Minecraft::GetSprintDelayTime()
{
    const u32 BaseAddr = GetBaseAddress(Base::Sprint);
    static u8 value;
    if (BaseAddr) {
        Process::Read8(BaseAddr + 0x1A28, value);
        return static_cast<float>(value) / 10.0f;
    } return 0.0f;
}

void Minecraft::SetSprintDelayTime(float seconds)
{
    const u32 BaseAddr = GetBaseAddress(Base::Player);
    if (BaseAddr) {
        u8 value = static_cast<u8>(seconds * 10.0f);
        Process::Write8(BaseAddr + 0x1A28, value);
    }
}

bool Minecraft::IsPlayerSprinting()
{
    const u32 BaseAddr = GetBaseAddress(Base::Sprint);
    static u8 value;
    if (BaseAddr) {
        Process::Read8(BaseAddr + 0x10, value);
        return value == 8;
    } return false;
}

void Minecraft::SetPlayerSprinting(bool enable)
{
    const u32 BaseAddr = GetBaseAddress(Base::Sprint);
    if (BaseAddr) {
        Process::Write8(BaseAddr + 0x10, enable ? 8 : 0);
    }
}

float Minecraft::GetPlayerMoveSpeed()
{
    const u32 BaseAddr = GetBaseAddress(Base::Speed);
    static float speed;
    if (BaseAddr) {
        Process::ReadFloat(BaseAddr + 0x4C, speed);
        return speed;
    } return 0.0f;
}

void Minecraft::SetPlayerMoveSpeed(float speed)
{
    const u32 BaseAddr = GetBaseAddress(Base::Speed);
    if (BaseAddr) {
        Process::WriteFloat(BaseAddr + 0x4C, speed);
    }
}

bool Minecraft::IsInvincible()
{
    const u32 BaseAddr1 = GetBaseAddress(Base::GameMode);
    static u32 BaseAddr2, BaseAddr3, BaseAddr4;
    static u8 value;
    if (BaseAddr1) {
        Process::Read32(BaseAddr1 + 0x14, BaseAddr2);
        Process::Read32(BaseAddr2 + 0x4, BaseAddr3);
        Process::Read32(BaseAddr3 + 0x8, BaseAddr4);
        Process::Write8(BaseAddr4 + 0x18, value);
        return value == 1;
    }
    return false;
}

void Minecraft::SetInvincible(bool enable)
{
    const u32 BaseAddr1 = GetBaseAddress(Base::GameMode);
    static u32 BaseAddr2, BaseAddr3, BaseAddr4;
    if (BaseAddr1) {
        Process::Read32(BaseAddr1 + 0x14, BaseAddr2);
        Process::Read32(BaseAddr2 + 0x4, BaseAddr3);
        Process::Read32(BaseAddr3 + 0x8, BaseAddr4);
        Process::Write8(BaseAddr4 + 0x18, enable ? 1 : 0);
    }
}

bool Minecraft::IsFlightAvailable()
{
    const u32 BaseAddr1 = GetBaseAddress(Base::GameMode);
    static u32 BaseAddr2, BaseAddr3;
    static u8 value;
    if (BaseAddr1) {
        Process::Read32(BaseAddr1 + 0x14, BaseAddr2);
        Process::Read32(BaseAddr2 + 0x4, BaseAddr3);
        Process::Read8(BaseAddr3 + 0x18, value);
        return value == 1;
    }
    return false;
}

void Minecraft::SetFlightAvailable(bool enable)
{
    const u32 BaseAddr1 = GetBaseAddress(Base::GameMode);
    static u32 BaseAddr2, BaseAddr3;
    if (BaseAddr1) {
        Process::Read32(BaseAddr1 + 0x14, BaseAddr2);
        Process::Read32(BaseAddr2 + 0x4, BaseAddr3);
        Process::Write8(BaseAddr3 + 0x18, enable ? 1 : 0);
    }
}

bool Minecraft::IsItemConsumptionDisabled()
{
    const u32 BaseAddr1 = GetBaseAddress(Base::GameMode);
    static u32 BaseAddr2, BaseAddr3;
    static u8 value;
    if (BaseAddr1) {
        Process::Read32(BaseAddr1 + 0x14, BaseAddr2);
        Process::Read32(BaseAddr2 + 0x4, BaseAddr3);
        Process::Read8(BaseAddr3 + 0x38, value);
        return value == 1;
    }
    return false;
}

void Minecraft::SetItemConsumptionDisabled(bool disable)
{
    const u32 BaseAddr1 = GetBaseAddress(Base::GameMode);
    static u32 BaseAddr2, BaseAddr3;
    if (BaseAddr1) {
        Process::Read32(BaseAddr1 + 0x14, BaseAddr2);
        Process::Read32(BaseAddr2 + 0x4, BaseAddr3);
        Process::Write8(BaseAddr3 + 0x38, disable ? 1 : 0);
    }
}

bool Minecraft::IsPlayerFlying()
{
    const u32 BaseAddr1 = GetBaseAddress(Base::GameMode);
    static u32 BaseAddr2, BaseAddr3, BaseAddr4;
    static u8 value;
    if (BaseAddr1) {
        Process::Read32(BaseAddr1 + 0x14, BaseAddr2);
        Process::Read32(BaseAddr2 + 0x4, BaseAddr3);
        Process::Read32(BaseAddr3 + 0x8, BaseAddr4);
        Process::Read8(BaseAddr4 + 0x38, value);
        return value == 1;
    } return false;
}

void Minecraft::SetPlayerFlying(bool enable)
{
    const u32 BaseAddr1 = GetBaseAddress(Base::GameMode);
    static u32 BaseAddr2, BaseAddr3, BaseAddr4;
    if (BaseAddr1) {
        Process::Read32(BaseAddr1 + 0x14, BaseAddr2);
        Process::Read32(BaseAddr2 + 0x4, BaseAddr3);
        Process::Read32(BaseAddr3 + 0x8, BaseAddr4);
        Process::Write8(BaseAddr4 + 0x38, enable ? 1 : 0);
    }
}

float Minecraft::GetFlightSpeed()
{
    const u32 BaseAddr1 = GetBaseAddress(Base::GameMode);
    static u32 BaseAddr2, BaseAddr3;
    static float speed;
    if (BaseAddr1) {
        Process::Read32(BaseAddr1 + 0x14, BaseAddr2);
        Process::Read32(BaseAddr2 + 0x4, BaseAddr3);
        Process::ReadFloat(BaseAddr3 + 0xA4, speed);
        return speed;
    } return 0.0f;
}

void Minecraft::SetFlightSpeed(float speed)
{
    if (speed < 0.01f) speed = 0.01f;
    else if (speed > 1.0f) speed = 1.0f;
    const u32 BaseAddr1 = GetBaseAddress(Base::GameMode);
    static u32 BaseAddr2, BaseAddr3;
    if (BaseAddr1) {
        Process::Read32(BaseAddr1 + 0x14, BaseAddr2);
        Process::Read32(BaseAddr2 + 0x4, BaseAddr3);
        Process::WriteFloat(BaseAddr3 + 0xA4, speed);
    }
}

float Minecraft::GetBaseMoveSpeed()
{
    const u32 BaseAddr1 = GetBaseAddress(Base::GameMode);
    static u32 BaseAddr2, BaseAddr3;
    static float speed;
    if (BaseAddr1) {
        Process::Read32(BaseAddr1 + 0x14, BaseAddr2);
        Process::Read32(BaseAddr2 + 0x4, BaseAddr3);
        Process::ReadFloat(BaseAddr3 + 0xC4, speed);
        return speed;
    } return 0.0f;
}

void Minecraft::SetBaseMoveSpeed(float speed)
{
    if (speed < 0.01f) speed = 0.01f;
    else if (speed > 1.0f) speed = 1.0f;
    const u32 BaseAddr1 = GetBaseAddress(Base::GameMode);
    static u32 BaseAddr2, BaseAddr3;
    if (BaseAddr1) {
        Process::Read32(BaseAddr1 + 0x14, BaseAddr2);
        Process::Read32(BaseAddr2 + 0x4, BaseAddr3);
        Process::WriteFloat(BaseAddr3 + 0xC4, speed);
    }
}

int Minecraft::GetGameMode()
{
    const u32 BaseAddr = GetBaseAddress(Base::Player);
    static u8 mode;
    if (BaseAddr) {
        Process::Read8(BaseAddr + 0x196C, mode);
        return mode;
    } return -1;
}

void Minecraft::SetGameMode(u8 mode)
{
    const u32 BaseAddr = GetBaseAddress(Base::Player);
    if (BaseAddr) {
        Process::Write8(BaseAddr + 0x196C, mode);
        switch (mode)
        {
            case 0: // survival
                SetInvincible(false);
                SetFlightAvailable(false);
                SetItemConsumptionDisabled(false);
                break;

            case 1: // creative
                SetInvincible(true);
                SetFlightAvailable(true);
                SetItemConsumptionDisabled(true);
                break;

            case 2: // adventure
                SetInvincible(false);
                SetFlightAvailable(false);
                SetItemConsumptionDisabled(false);
                break;

            default:
                break;
        }
    } return;
}

float Minecraft::GetYaw()
{
    const u32 BaseAddr = GetBaseAddress(Base::Player);
    static float yaw;
    if (BaseAddr) {
        Process::ReadFloat(BaseAddr + 0x1F4, yaw);
        return yaw;
    } return 0.0f;
}

void Minecraft::SetYaw(float yaw)
{
    const u32 BaseAddr = GetBaseAddress(Base::Player);
    if (BaseAddr) {
        Process::WriteFloat(BaseAddr + 0x1F4, yaw);
    }
}

float Minecraft::GetPitch()
{
    const u32 BaseAddr = GetBaseAddress(Base::Player);
    static float pitch;
    if (BaseAddr) {
        Process::ReadFloat(BaseAddr + 0x1F0, pitch);
        return pitch;
    } return 0.0f;
}

void Minecraft::SetPitch(float pitch)
{
    const u32 BaseAddr = GetBaseAddress(Base::Player);
    static float minPitch, maxPitch;
    if (BaseAddr) {
        Process::ReadFloat(0x005F6354, minPitch);
        Process::ReadFloat(0x005F635C, maxPitch);
        if (pitch < minPitch) pitch = minPitch;
        if (pitch > maxPitch) pitch = maxPitch;
        Process::WriteFloat(BaseAddr + 0x1F0, pitch);
    }
}

float Minecraft::GetVelocityX() {
    const u32 BaseAddr = GetBaseAddress(Base::Player);
    static float x;
    if (BaseAddr) {
        Process::ReadFloat(BaseAddr + 0x1E4, x);
        return x;
    } return 0.0f;
}

float Minecraft::GetVelocityY() {
    const u32 BaseAddr = GetBaseAddress(Base::Player);
    static float y;
    if (BaseAddr) {
        Process::ReadFloat(BaseAddr + 0x1E8, y);
        return y;
    } return 0.0f;
}

float Minecraft::GetVelocityZ() {
    const u32 BaseAddr = GetBaseAddress(Base::Player);
    static float z;
    if (BaseAddr) {
        Process::ReadFloat(BaseAddr + 0x1EC, z);
        return z;
    } return 0.0f;
}

void Minecraft::SetVelocityX(float x) {
    const u32 BaseAddr = GetBaseAddress(Base::Player);
    if (BaseAddr) {
        Process::WriteFloat(BaseAddr + 0x1E4, x);
    }
}

void Minecraft::SetVelocityY(float y) {
    const u32 BaseAddr = GetBaseAddress(Base::Player);
    if (BaseAddr) {
        Process::WriteFloat(BaseAddr + 0x1E8, y);
    }
}

void Minecraft::SetVelocityZ(float z) {
    const u32 BaseAddr = GetBaseAddress(Base::Player);
    if (BaseAddr) {
        Process::WriteFloat(BaseAddr + 0x1EC, z);
    }
}

bool Minecraft::IsPlayerUnderWater()
{
    static u32 BaseAddr;
    static u8 value;
    Process::Read32(0x00A32244, BaseAddr);
    if (BaseAddr) {
        Process::Read8(BaseAddr + 0x381, value);
        return value == 1;
    } return false;
}

void Minecraft::SetPlayerUnderWater(bool enable)
{
    static u32 BaseAddr;
    Process::Read32(0x00A32244, BaseAddr);
    if (BaseAddr) {
        Process::Write8(BaseAddr + 0x380, enable ? 1 : 0);
    }
}

int Minecraft::GetJumpCooldown()
{
    const u32 BaseAddr = GetBaseAddress(Base::Player);
    static u8 value;
    if (BaseAddr) {
        Process::Read8(BaseAddr + 0x107C, value);
        return value;
    } return 0;
}

void Minecraft::SetJumpCooldown(int cooldown)
{
    const u32 BaseAddr = GetBaseAddress(Base::Player);
    if (BaseAddr) {
        Process::Write8(BaseAddr + 0x107C, cooldown);
    }
}

int Minecraft::GetDamageCooldown()
{
    const u32 BaseAddr = GetBaseAddress(Base::Player);
    static u8 value;
    if (BaseAddr) {
        Process::Read8(BaseAddr + 0x2F0, value);
        return value;
    } return 0;
}

void Minecraft::SetDamageCooldown(int cooldown)
{
    const u32 BaseAddr = GetBaseAddress(Base::Player);
    if (BaseAddr) {
        Process::Write8(BaseAddr + 0x2F0, cooldown);
    }
}

bool Minecraft::IsPlayerTouchingWall()
{
    const u32 BaseAddr = GetBaseAddress(Base::Player);
    static u8 value;
    if (BaseAddr) {
        Process::Read8(BaseAddr + 0x25C, value);
        return value == 1;
    } return false;
}

void Minecraft::SetPlayerTouchingWall(bool enable)
{
    const u32 BaseAddr = GetBaseAddress(Base::Player);
    if (BaseAddr) {
        Process::Write8(BaseAddr + 0x25C, enable ? 1 : 0);
    }
}

float Minecraft::GetAirMoveSpeed()
{
    const u32 BaseAddr = GetBaseAddress(Base::Player);
    static float speed;
    if (BaseAddr) {
        Process::ReadFloat(BaseAddr + 0x1038, speed);
        return speed;
    } return 0.0f;
}

void Minecraft::SetAirMoveSpeed(float speed)
{
    const u32 BaseAddr = GetBaseAddress(Base::Player);
    if (BaseAddr) {
        Process::WriteFloat(BaseAddr + 0x1038, speed);
    }
}

//========================
// WorldUtils
//========================

bool Minecraft::IsRaining()
{
    const u32 BaseAddr = GetBaseAddress(Base::Weather);
    static float value;
    if (BaseAddr) {
        Process::ReadFloat(BaseAddr + 0x20, value);
        return value != 0;
    } return false;
}

void Minecraft::SetRain(bool enable)
{
    const u32 BaseAddr = GetBaseAddress(Base::Weather);
    if (BaseAddr) {
        Process::WriteFloat(BaseAddr + 0x20, enable ? 1.0f : 0.0f);
    }
}

bool Minecraft::IsThundering()
{
    const u32 BaseAddr = GetBaseAddress(Base::Weather);
    static float value;
    if (BaseAddr) {
        Process::ReadFloat(BaseAddr + 0x2C, value);
        return value != 0;
    } return false;
}

void Minecraft::SetThunder(bool enable)
{
    const u32 BaseAddr = GetBaseAddress(Base::Weather);
    if (BaseAddr) {
        Process::WriteFloat(BaseAddr + 0x2C, enable ? 1.0f : 0.0f);
    }
}

