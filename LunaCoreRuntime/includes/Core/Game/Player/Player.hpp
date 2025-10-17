#pragma once

#include "lua_common.h"
#include "types.h"

#include "Game/Inventory.hpp"

namespace Core {
    class Player {
        using InventorySlot = Game::Inventory::InventorySlot;
        typedef struct {
            float x, y, z;
        } coordsComponents;

        typedef struct {
            float pitch, yaw;
        } cameraAngle;

        public:
        char padding1[0x198];
        u8 dimId; // 0x198
        char padding2[0x1c0 - 0x198 - sizeof(u8)];
        coordsComponents coords1, coords2, coords3; // 0x1c0
        coordsComponents velocity; // 0x1e4
        cameraAngle camAngle; // 0x1f0
        char padding3[0x288 - 0x1f0 - sizeof(cameraAngle)];
        coordsComponents coords4, coords5; // 0x288
        char padding4[0x2f0 - 0x288 - sizeof(coordsComponents) * 2];
        u8 damageCooldown; // 0x2f0
        char padding5[0xf40 - 0x2f0 - sizeof(u8)];
        InventorySlot armorSlots[4]; // 0xf40
        char padding6[0x107c - 0xf40 - sizeof(InventorySlot) * 4];
        u8 jumpCooldown; // 0x107c
        char padding7[0x196c - 0x107c - sizeof(u8)];
        u8 gamemode; // 0x196c
        char padding8[0x1a28 - 0x196c - sizeof(u8)];
        u8 sprintDelay; // 0x1a28
        char padding9[0x28b8 - 0x1a28 - sizeof(u8)];

        static inline Player** PlayerInstance = reinterpret_cast<Player**>(0x918958);

        void setPosition(float x, float y, float z) {
            this->coords1.x = x;
            this->coords4.x = x;
            this->coords5.x = x;
            
            this->coords1.y = y;
            this->coords4.y = y;
            this->coords5.y = y;

            this->coords1.z = z;
            this->coords4.z = z;
            this->coords5.z = z;
        }
    };

    namespace Module {
        bool RegisterLocalPlayerModule(lua_State *L);
    }
}