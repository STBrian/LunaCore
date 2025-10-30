#include "Core/Game/Entity.hpp"

#include "lua_object.hpp"

#include "Minecraft/world/entity/Entity.hpp"

using Entity = Game::Entity;

// ----------------------------------------------------------------------------

//@@GameEntity
//@@GameSpawnCoords

// ----------------------------------------------------------------------------

/*
=GameEntity.X = 0.0
=GameEntity.Y = 0.0
=GameEntity.Z = 0.0
=GameEntity.EntityID = 0

=GameSpawnCoords.X = 0.0
=GameSpawnCoords.Y = 0.0
=GameSpawnCoords.Z = 0.0
*/

static const LuaObjectField GameEntityFields[] = {
    {"X", OBJF_TYPE_FLOAT, offsetof(Entity, x)},
    {"Y", OBJF_TYPE_FLOAT, offsetof(Entity, y)},
    {"Z", OBJF_TYPE_FLOAT, offsetof(Entity, z)},
    {"EntityID", OBJF_TYPE_INT, offsetof(Entity, entityTypeId), OBJF_ACCESS_INDEX},
    {NULL, OBJF_TYPE_NIL, 0}
};

static const LuaObjectField GameSpawnCoordsFields[] = {
    {"X", OBJF_TYPE_FLOAT, 0},
    {"Y", OBJF_TYPE_FLOAT, 4},
    {"Z", OBJF_TYPE_FLOAT, 8},
    {NULL, OBJF_TYPE_NIL, 0}
};

bool Core::Module::RegisterEntityModule(lua_State *L) {
    LuaObject::RegisterNewObject(L, "GameEntity", GameEntityFields);
    LuaObject::RegisterNewObject(L, "GameSpawnCoords", GameSpawnCoordsFields);
    return true;
}