#include "LuaModules.hpp"

#include "game/entity/Entity.hpp"
#include "Helpers/LuaObject.hpp"

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

typedef struct {
    float x, y, z;
} GameSpawnCoords;

bool Core::Module::RegisterEntityModule(lua_State *L) {
    ClassBuilder<Entity>(L, "GameEntity")
        .property("X", &Entity::x)
        .property("Y", &Entity::y)
        .property("Z", &Entity::z)
        .property("EntityID", &Entity::entityTypeId, true)
        .build();
    ClassBuilder<GameSpawnCoords>(L, "GameSpawnCoords")
        .property("X", &GameSpawnCoords::x)
        .property("Y", &GameSpawnCoords::y)
        .property("Z", &GameSpawnCoords::z)
        .build();
    return true;
}