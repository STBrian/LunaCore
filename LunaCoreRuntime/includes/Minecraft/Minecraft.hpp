//Minecraft class v1.1 by: rairai6895
#pragma once
#include <CTRPluginFramework.hpp>

#include <optional>

using namespace CTRPluginFramework;
namespace Minecraft {

    enum class Base : u8 {
        Player,
        Status,
        Level,
        ItemSlot,
        HeldSlot,
        JumpSneak,
        Speed,
        Sprint,
        GameMode,
        Weather
    };
     void InitBaseAddress();
      u32 GetBaseAddress(const Base base);
      void PatchProcess();

    //========================
    // InventryUtils
    //========================

    struct  ItemData
    {
        u8 count;        // +0x00
        u8  unk1 = 0;         // +0x01
        u16 dataValue = 0;    // +0x02
        u32 unk2 = 0;         // +0x04
        u32 enchant = 0;      // +0x08
        u32 itemID = 0;       // +0x0C
        u32 renderID = 0;     // +0x10
        u8  unk3[24] = {};     // +0x14 ~ 0x2B
    };

    // スロットのアドレスを取得する関数
      u32 GetSlotAddress(int slotNum);

    // 指定したスロットのアイテムIDを取得する関数
      u32 GetItemID(int slotNum);

    // 指定したスロットのアイテムIDを設定する関数
      void SetItemID(int slotNum, u32 itemID);

    // 指定したスロットのアイテム個数を取得する関数
      int GetItemCount(int slotNum);

    // 指定したスロットのアイテム個数を設定する関数
      void SetItemCount(int slotNum, int count);

    // 指定したスロットのアイテムデータ値を取得する関数
      u16 GetItemData(int slotNum);

    // 指定したスロットのアイテムデータ値を設定する関数
      void SetItemData(int slotNum, u16 dataValue);

    // 指定した2つのスロットのアイテムを交換する関数
      void SwapSlotItems(int slotA, int slotB);

    // 全スロットのアイテムをスキャンして特定のアイテムが見つかったアドレスを返す関数
      u32 FindItemAddress(u32 targetItemID, u16 targetDataValue, bool useDataValue);

    // 特定のアイテムIDと一致するスロットのアドレスをリストとして返す関数
      std::vector<u32> FindAllItemAddresses(u32 targetItemID, u16 targetDataValue, bool useDataValue);

    // 特定のアイテムIDと一致するスロットの中で最小のアドレスを返す関数
      u32 FindSmallestItemAddress(u32 targetItemID, u16 targetDataValue, bool useDataValue);

    // アドレスからスロットの数字を返す関数
      int GetSlotNumberFromAddress(u32 address);

    // 現在選択しているスロットの数字を返す関数
      int GetHeldSlotNumber();

    // 現在選択しているスロットのアドレスを返す関数
      u32 GetHeldSlotAddress();

    // スロットを選択させる関数
      void SetHeldSlotNumber(int slotNum);

    // インベントリ内に特定のアイテムが何個あるかを返す関数
      int GetCountItemInInventory(u32 targetItemID, u16 targetDataValue, bool useDataValue);

    // アイテムを付与する関数
      int GiveItems(ItemData data);

    struct ItemEntry {
        std::string name;
        u32 idAddress;
        u16 dataValue;
    };

    extern std::vector<ItemEntry> itemList;

      void AddItemEntry(const std::string& name, u32 idAddress, u16 dataValue = 0);
      std::vector<ItemEntry*> FindItemByName(const std::string& partialName);
      const std::vector<ItemEntry>& GetItemList();

    struct SelectedItemInfo
    {
        std::string name;
        u32 itemIDAddress;
        u32 itemID;
        u16 dataValue;
        u32 renderID;
    };

    // 検索機能付きアイテムリストを表示する関数
      std::optional<SelectedItemInfo> ShowItemSelector
    (
        const std::string& searchTitle = "Search...",
        const std::string& resultTitle = "Search results"
    );

    // ページング付き＆検索機能付きアイテムリストを表示する関数
      std::optional<Minecraft::SelectedItemInfo> ShowItemSelectorWithPaging
    (
        const std::string& searchTitle = "Search...",
        const std::string& resultTitle  = "Search results",
        const std::string& pagingNextTitle = "Next >",
        const std::string& pagingBackTitle = "< Back"
    );

    // 
      u32 GetRenderID(u32 idAddress);

    // 防具スロットのアドレスを取得する関数
      u32 GetArmorSlotAddress(int slotNum);

    // 指定した防具スロットのアイテムIDを取得する関数
      u32 GetArmorItemID(int slotNum);

    // 指定した防具スロットのアイテムIDを設定する関数
      void SetArmorItemID(int slotNum, u32 itemID);

    // 指定した防具スロットのアイテム個数を取得する関数
      int GetArmorItemCount(int slotNum);

    // 指定した防具スロットのアイテム個数を設定する関数
      void SetArmorItemCount(int slotNum, int count);

    // 指定した防具スロットのアイテムデータ値を取得する関数
      u16 GetArmorItemData(int slotNum);

    // 指定した防具スロットのアイテムデータ値を設定する関数
      void SetArmorItemData(int slotNum, u16 dataValue);

    // 指定したスロットのアイテムと防具スロットのアイテムを交換する関数
      void SwapArmorAndInventoryItem(int armorSlot, int inventorySlot);

    //========================
    // PlayerUtils
    //========================

    // 体力値を取得する関数
      float GetCurrentHP();

    // 最大体力値を取得する関数
      float GetMaxHP();

    // 体力値を設定する関数
      void SetCurrentHP(float value);

    // 最大体力値を設定する関数
      void SetMaxHP(float value);

    // 空腹値を取得する関数
      float GetCurrentHunger();

    // 最大空腹値を取得する関数
      float GetMaxHunger();

    // 空腹値を設定する関数
      void SetCurrentHunger(float value);

    // 最大空腹値を設定する関数
      void SetMaxHunger(float value);

    // レベルを取得する関数
      float GetCurrentLevel();

    // レベルを設定する関数
      void SetCurrentLevel(float newLevel);

    // 経験値バーの進捗を取得する関数
      float GetXPBarProgress();

    // 経験値バーの進捗を設定する関数
      void SetXPBarProgress(float progress);

    // プレイヤーの座標を取得する関数
      void GetPlayerPosition(float& posX, float& posY, float& posZ);

    // プレイヤーの座標を設定する関数
      void SetPlayerPosition(float posX, float posY, float posZ);

    // 地面についているかを取得する関数
      bool IsPlayerOnGround();

    // 地面についてるかを設定する関数
      void SetPlayerOnGround(bool onGround);

    // しゃがんでいるかを取得する関数
      bool IsPlayerSneaking();

    // しゃがみ状態を設定する関数
      void SetPlayerSneaking(bool enable);

    // ジャンプ入力をしているかを取得する関数
      bool IsPlayerJumping();

    // ジャンプ入力を設定する関数
      void SetPlayerJump(bool enable);

    // ダッシュ可能時間を取得する関数
      float GetSprintDelayTime();

    // ダッシュ可能時間を設定する関数
      void SetSprintDelayTime(float seconds);

    // プレイヤーがダッシュしているかを取得する関数
      bool IsPlayerSprinting();

    // プレイヤーがダッシュしているかを設定する関数
      void SetPlayerSprinting(bool enable);

    // プレイヤーの移動速度を取得する関数
      float GetPlayerMoveSpeed();

    // プレイヤーの移動速度を設定する関数
      void SetPlayerMoveSpeed(float speed);

    // 無敵状態かを取得する関数
      bool IsInvincible();

    // 無敵状態かを設定する関数
      void SetInvincible(bool enable);

    // 飛行が可能かを取得する関数
      bool IsFlightAvailable();

    // 飛行が可能かを設定する関数
      void SetFlightAvailable(bool enable);

    // アイテム消費が無効かを取得する関数
      bool IsItemConsumptionDisabled();

    // アイテム消費が無効かを設定する関数
      void SetItemConsumptionDisabled(bool disable);

    // 飛行しているかを取得する関数
      bool IsPlayerFlying();

    // 飛行しているかを設定する関数
      void SetPlayerFlying(bool enable);

    // 飛行速度を取得する関数
      float GetFlightSpeed();

    // 飛行速度を設定する関数
      void SetFlightSpeed(float speed);

    // ベース移動速度を取得する関数
      float GetBaseMoveSpeed();

    // ベース移動速度を設定する関数
      void SetBaseMoveSpeed(float speed);

    // 現在のゲームモードを取得する関数
      int GetGameMode();

    // 現在のゲームモードを設定する関数
      void SetGameMode(u8 mode);

    // 現在の視点Xを取得する関数
      float GetYaw();

    // 現在の視点Xを設定する関数
      void SetYaw(float yaw);

    // 現在の視点Yを取得する関数
      float GetPitch();

    // 現在の視点Yを設定する関数
      void SetPitch(float pitch);

    // X軸の移動速度を取得する関数
      float GetVelocityX();

    // Y軸の移動速度を取得する関数
      float GetVelocityY();

    // Z軸の移動速度を取得する関数
      float GetVelocityZ();

    // X軸の移動速度を設定する関数
      void SetVelocityX(float x);

    // Y軸の移動速度を設定する関数
      void SetVelocityY(float y);

    // Z軸の移動速度を設定する関数
      void SetVelocityZ(float z);

    // 水面下にいるかを取得する関数
      bool IsPlayerUnderWater();

    // 水面下にいるかを設定する関数
      void SetPlayerUnderWater(bool enable);

    // ジャンプのクールダウン時間を取得する関数
      int GetJumpCooldown();

    // ジャンプのクールダウン時間を設定する関数
      void SetJumpCooldown(int cooldown);

    // 攻撃を受けた後の無敵時間を取得する関数
      int GetDamageCooldown();

    // 攻撃を受けた後の無敵時間を設定する関数
      void SetDamageCooldown(int cooldown);

    // 壁に触れているかを取得する関数
      bool IsPlayerTouchingWall();

    // 壁に触れているかを設定する関数
      void SetPlayerTouchingWall(bool enable);

    // 空中での移動速度を取得する関数
      float GetAirMoveSpeed();

    // 空中での移動速度を設定する関数
      void SetAirMoveSpeed(float speed);

    //========================
    // WorldUtils
    //========================

    // 雨が降っているかを取得する関数
      bool IsRaining();

    // 雨が降っているかを設定する関数
      void SetRain(bool enable);

    // 雷が降っているかを取得する関数
      bool IsThundering();
    
    // 雷が降っているかを設定する関数
      void SetThunder(bool enable);

}