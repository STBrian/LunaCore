#pragma once

#ifndef NOSTDLIB_BUILD
#include <string>
#endif

#include "game/asserts.h"

//#include "Minecraft/game_utils/game_functions.hpp"
#include "game/gstd/gstd_string.hpp"
#include "game/world/item/ItemInstance.hpp"

namespace Game {
    class Item;
    
    static Item*(*registerItem)(const char* nameId, const short& itemId) = reinterpret_cast<decltype(registerItem)>(0x007cdc8c);

    class Item {
        public:
        //void* vtable; // 0x0
        u8 maxStackSize; // 0x4
        //char unk1[3]; // 0x5
        gstd::string atlasName; // 0x8
        int unk2; // 0xc
        char unk3; // 0x10
        u16 itemId; // 0x12
        gstd::string descriptionId; // 0x14
        gstd::string nameId; // 0x18
        short unk4; // 0x1c
        u16 durability; // 0x1e
        bool enchantEffect; // 0x20
        short unk5; // 0x22
        bool disablePlacement; // 0x24
        short unk6; // 0x26
        u8 blockID; // 0x28
        bool canEat; // 0x29
        u8 padding[0xac - 0x2a];

        class Tier {
            public:
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
        inline static ItemInstance** creativeItems = reinterpret_cast<ItemInstance**>(0x00b0d744);
        inline static ItemInstance** creativeItemsEnd = reinterpret_cast<ItemInstance**>(0x00b0d748);

        inline static void (*registerItems)() = reinterpret_cast<void(*)()>(0x00563db0);

        #ifndef NOSTDLIB_BUILD
        /* Item.itemId = itemId + 0x100 */
        Item(const std::string& nameId, short itemId) {
            reinterpret_cast<void*(*)(Item*, const gstd::string&, short)>(0x005790a4)(this, nameId, itemId);
        }
        #endif

        /* Item.itemId = itemId + 0x100 */
        Item(const gstd::string& nameId, short itemId) {
            reinterpret_cast<void*(*)(Item*, const gstd::string&, short)>(0x005790a4)(this, nameId, itemId);
        }

        inline static void addCreativeItem(Item* item, u8 categoryId, s16 position) {
            ItemInstance itemins(item->itemId, 1, 0); // Def values
            itemins.unknown1 = categoryId;
            itemins.unknown2 = position;
            reinterpret_cast<void(*)(ItemInstance*)>(0x0056e108)(&itemins);
        }

        virtual void unkFunc1();
        virtual void unkFunc2();
        virtual void setTexture(u32 texnameHash, u32 idx);

        inline static Item** mItemTable = reinterpret_cast<Item**>(0x00a33f40);
        inline static Item*& mShovel_iron = mItemTable[1];
        inline static Item*& mPickAxe_iron = mItemTable[2];
        inline static Item*& mHatchet_iron = mItemTable[3];
        inline static Item*& mFlintAndSteel = mItemTable[4];
        inline static Item*& mApple = mItemTable[5];
        inline static Item*& mBow = mItemTable[6];
        inline static Item*& mArrow = mItemTable[7];
        inline static Item*& mCoal = mItemTable[8];
        inline static Item*& mDiamond = mItemTable[9];
        inline static Item*& mIronIngot = mItemTable[10];
        inline static Item*& mGoldIngot = mItemTable[11];
        inline static Item*& mSword_iron = mItemTable[12];
        inline static Item*& mSword_wood = mItemTable[13];
        inline static Item*& mShovel_wood = mItemTable[14];
        inline static Item*& mPickAxe_wood = mItemTable[15];
        inline static Item*& mHatchet_wood = mItemTable[16];
        inline static Item*& mSword_stone = mItemTable[17];
        inline static Item*& mShovel_stone = mItemTable[18];
        inline static Item*& mPickAxe_stone = mItemTable[19];
        inline static Item*& mHatchet_stone = mItemTable[20];
        inline static Item*& mSword_diamond = mItemTable[21];
        inline static Item*& mShovel_diamond = mItemTable[22];
        inline static Item*& mPickAxe_diamond = mItemTable[23];
        inline static Item*& mHatchet_diamond = mItemTable[24];
        inline static Item*& mStick = mItemTable[25];
        inline static Item*& mBowl = mItemTable[26];
        inline static Item*& mMushroomStew = mItemTable[27];
        inline static Item*& mSword_gold = mItemTable[28];
        inline static Item*& mShovel_gold = mItemTable[29];
        inline static Item*& mPickAxe_gold = mItemTable[30];
        inline static Item*& mHatchet_gold = mItemTable[31];
        inline static Item*& mString = mItemTable[32];
        inline static Item*& mFeather = mItemTable[33];
        inline static Item*& mSulphur = mItemTable[34];
        inline static Item*& mHoe_wood = mItemTable[35];
        inline static Item*& mHoe_stone = mItemTable[36];
        inline static Item*& mHoe_iron = mItemTable[37];
        inline static Item*& mHoe_diamond = mItemTable[38];
        inline static Item*& mHoe_gold = mItemTable[39];
        inline static Item*& mSeeds_wheat = mItemTable[40];
        inline static Item*& mWheat = mItemTable[41];
        inline static Item*& mBread = mItemTable[42];
        inline static Item*& mHelmet_leather = mItemTable[43];
        inline static Item*& mChestplate_leather = mItemTable[44];
        inline static Item*& mLeggings_leather = mItemTable[45];
        inline static Item*& mBoots_leather = mItemTable[46];
        inline static Item*& mElytra = mItemTable[47];
        inline static Item*& mHelmet_chain = mItemTable[48];
        inline static Item*& mChestplate_chain = mItemTable[49];
        inline static Item*& mLeggings_chain = mItemTable[50];
        inline static Item*& mBoots_chain = mItemTable[51];
        inline static Item*& mHelmet_iron = mItemTable[52];
        inline static Item*& mChestplate_iron = mItemTable[53];
        inline static Item*& mLeggings_iron = mItemTable[54];
        inline static Item*& mBoots_iron = mItemTable[55];
        inline static Item*& mHelmet_diamond = mItemTable[56];
        inline static Item*& mChestplate_diamond = mItemTable[57];
        inline static Item*& mLeggings_diamond = mItemTable[58];
        inline static Item*& mBoots_diamond = mItemTable[59];
        inline static Item*& mHelmet_gold = mItemTable[60];
        inline static Item*& mChestplate_gold = mItemTable[61];
        inline static Item*& mLeggings_gold = mItemTable[62];
        inline static Item*& mBoots_gold = mItemTable[63];
        inline static Item*& mFlint = mItemTable[64];
        inline static Item*& mPorkChop_raw = mItemTable[65];
        inline static Item*& mPorkChop_cooked = mItemTable[66];
        inline static Item*& mPainting = mItemTable[67];
        inline static Item*& mApple_gold = mItemTable[68];
        inline static Item*& mApple_enchanted = mItemTable[69];
        inline static Item*& mSign = mItemTable[70];
        inline static Item*& mDoor_wood = mItemTable[71];
        inline static Item*& mBucket = mItemTable[72];
        inline static Item*& mMinecart = mItemTable[73];
        inline static Item*& mSaddle = mItemTable[74];
        inline static Item*& mDoor_iron = mItemTable[75];
        inline static Item*& mRedStone = mItemTable[76];
        inline static Item*& mSnowBall = mItemTable[77];
        inline static Item*& mBoat = mItemTable[78];
        inline static Item*& mLeather = mItemTable[79];
        inline static Item*& mRabbitHide = mItemTable[80];
        inline static Item*& mBrick = mItemTable[81];
        inline static Item*& mClay = mItemTable[82];
        inline static Item*& mReeds = mItemTable[83];
        inline static Item*& mPaper = mItemTable[84];
        inline static Item*& mBook = mItemTable[85];
        inline static Item*& mSlimeBall = mItemTable[86];
        inline static Item*& mChestMinecart = mItemTable[87];
        inline static Item*& mEgg = mItemTable[88];
        inline static Item*& mCompass = mItemTable[89];
        inline static Item*& mFishingRod = mItemTable[90];
        inline static Item*& mClock = mItemTable[91];
        inline static Item*& mYellowDust = mItemTable[92];
        inline static Item*& mCarrotOnAStick = mItemTable[93];
        inline static Item*& mFish_raw_cod = mItemTable[94];
        inline static Item*& mFish_raw_salmon = mItemTable[95];
        inline static Item*& mFish_raw_clownfish = mItemTable[96];
        inline static Item*& mFish_raw_pufferfish = mItemTable[97];
        inline static Item*& mFish_cooked_cod = mItemTable[98];
        inline static Item*& mFish_cooked_salmon = mItemTable[99];
        inline static Item*& mMelonSlice = mItemTable[100];
        inline static Item*& mSpeckled_melon = mItemTable[101];
        inline static Item*& mSeeds_pumpkin = mItemTable[102];
        inline static Item*& mSeeds_melon = mItemTable[103];
        inline static Item*& mEnderpearl = mItemTable[104];
        inline static Item*& mEnderEye = mItemTable[105];
        inline static Item*& mEndCrystal = mItemTable[106];
        inline static Item*& mBlazeRod = mItemTable[107];
        inline static Item*& mNether_wart = mItemTable[108];
        inline static Item*& mGold_nugget = mItemTable[109];
        inline static Item*& mIron_nugget = mItemTable[110];
        inline static Item*& mSpider_eye = mItemTable[111];
        inline static Item*& mFermented_spider_eye = mItemTable[112];
        inline static Item*& mBlaze_powder = mItemTable[113];
        inline static Item*& mMobPlacer = mItemTable[114];
        inline static Item*& mExperiencePotionItem = mItemTable[115];
        inline static Item*& mFireCharge = mItemTable[116];
        inline static Item*& mDye_powder = mItemTable[117];
        inline static Item*& mBone = mItemTable[118];
        inline static Item*& mSugar = mItemTable[119];
        inline static Item*& mCake = mItemTable[120];
        inline static Item*& mBed = mItemTable[121];
        inline static Item*& mRepeater = mItemTable[122];
        inline static Item*& mCookie = mItemTable[123];
        inline static Item*& mPumpkinPie = mItemTable[124];
        inline static Item*& mPotato = mItemTable[125];
        inline static Item*& mPotatoBaked = mItemTable[126];
        inline static Item*& mPoisonous_potato = mItemTable[127];
        inline static Item*& mCarrot = mItemTable[128];
        inline static Item*& mGoldenCarrot = mItemTable[129];
        inline static Item*& mBeetroot = mItemTable[130];
        inline static Item*& mSeeds_beetroot = mItemTable[131];
        inline static Item*& mBeetrootSoup = mItemTable[132];
        inline static Item*& mSkull = mItemTable[133];
        inline static Item*& mNetherStar = mItemTable[134];
        inline static Item*& mShears = mItemTable[135];
        inline static Item*& mBeef_raw = mItemTable[136];
        inline static Item*& mBeef_cooked = mItemTable[137];
        inline static Item*& mChicken_raw = mItemTable[138];
        inline static Item*& mChicken_cooked = mItemTable[139];
        inline static Item*& mMutton_raw = mItemTable[140];
        inline static Item*& mMutton_cooked = mItemTable[141];
        inline static Item*& mRotten_flesh = mItemTable[142];
        inline static Item*& mRabbitRaw = mItemTable[143];
        inline static Item*& mRabbitCooked = mItemTable[144];
        inline static Item*& mRabbitStew = mItemTable[145];
        inline static Item*& mRabbitFoot = mItemTable[146];
        inline static Item*& mLeatherHorseArmor = mItemTable[147];
        inline static Item*& mIronHorseArmor = mItemTable[148];
        inline static Item*& mGoldHorseArmor = mItemTable[149];
        inline static Item*& mDiamondHorseArmor = mItemTable[150];
        inline static Item*& mLead = mItemTable[151];
        inline static Item*& mNameTag = mItemTable[152];
        inline static Item*& mDoor_spruce = mItemTable[153];
        inline static Item*& mDoor_birch = mItemTable[154];
        inline static Item*& mDoor_jungle = mItemTable[155];
        inline static Item*& mDoor_acacia = mItemTable[156];
        inline static Item*& mDoor_darkoak = mItemTable[157];
        inline static Item*& mGhast_tear = mItemTable[158];
        inline static Item*& mMagma_cream = mItemTable[159];
        inline static Item*& mBrewing_stand = mItemTable[160];
        inline static Item*& mCauldron = mItemTable[161];
        inline static Item*& mEnchanted_book = mItemTable[162];
        inline static Item*& mComparator = mItemTable[163];
        inline static Item*& mNetherbrick = mItemTable[164];
        inline static Item*& mNetherQuartz = mItemTable[165];
        inline static Item*& mTNTMinecart = mItemTable[166];
        inline static Item*& mCommandBlockMinecart = mItemTable[167];
        inline static Item*& mPrismarineShard = mItemTable[168];
        inline static Item*& mPrismarineCrystal = mItemTable[169];
        inline static Item*& mEmerald = mItemTable[170];
        inline static Item*& mItemFrame = mItemTable[171];
        inline static Item*& mFlowerPot = mItemTable[172];
        inline static Item*& mPotion = mItemTable[173];
        inline static Item*& mSplash_potion = mItemTable[174];
        inline static Item*& mLingering_potion = mItemTable[175];
        inline static Item*& mGlass_bottle = mItemTable[176];
        inline static Item*& mDragon_breath = mItemTable[177];
        inline static Item*& mHopperMinecart = mItemTable[178];
        inline static Item*& mHopper = mItemTable[179];
        inline static Item*& mFilledMap = mItemTable[180];
        inline static Item*& mEmptyMap = mItemTable[181];
        inline static Item*& mShulkerShell = mItemTable[182];
        inline static Item*& mTotem = mItemTable[183];

        inline static Item*& mChorusFruit = mItemTable[186];
        inline static Item*& mChorusFruitPopped = mItemTable[187];
    };

    ASSERT_SIZE(Game::Item, 0xac);
}