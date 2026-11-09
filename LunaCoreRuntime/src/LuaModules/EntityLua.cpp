#include "LuaModules.hpp"

#include "game/entity/Entity.hpp"
#include "Helpers/LuaObject.hpp"

using Entity = Game::Entity;

// ----------------------------------------------------------------------------

/* @doc:v2
@class GameEntity
@desc Represents an entity in the game world
@field X number
@field Y number
@field Z number
@field EntityID integer
*/

/* @doc:v2
@class GameSpawnCoords
@desc Coordinates used for spawning entities
@field X number
@field Y number
@field Z number
*/

/* @doc:v2
@module Game.Entity
@desc Game Entity module
*/

// ----------------------------------------------------------------------------

/* @doc:v2
@const GameEntity.X = 0.0
@const GameEntity.Y = 0.0
@const GameEntity.Z = 0.0
@const GameEntity.EntityID = 0

@const GameSpawnCoords.X = 0.0
@const GameSpawnCoords.Y = 0.0
@const GameSpawnCoords.Z = 0.0
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

    lua_getglobal(L, "Game");
    lua_newtable(L);

    lua_setfield(L, -2, "Entity");
    lua_pop(L, 1);
    return true;
}