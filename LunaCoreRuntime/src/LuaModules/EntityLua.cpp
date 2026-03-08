#include "LuaModules.hpp"

#include "game/entity/Entity.hpp"
#include "Helpers/LuaObject.hpp"

using Entity = Game::Entity;

// ----------------------------------------------------------------------------

//@@GameEntity
//@@GameSpawnCoords
//$Game.Entity
//@EntityState: integer

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

    lua_getglobal(L, "Game");
    lua_newtable(L);

    //$Game.Entity.States
    lua_newtable(L);
    //=Game.Entity.States.Burning = 0
    luaC_setfield_integer(L, 0, "Burning");
    //=Game.Entity.States.Sneaking = 1
    luaC_setfield_integer(L, 1, "Sneaking");
    //=Game.Entity.States.Sprinting = 3
    luaC_setfield_integer(L, 3, "Sprinting");
    //=Game.Entity.States.Eating = 4
    luaC_setfield_integer(L, 4, "Eating");
    //=Game.Entity.States.Invisible = 5
    luaC_setfield_integer(L, 5, "Invisible");
    //=Game.Entity.States.IsTempted = 6
    luaC_setfield_integer(L, 6, "IsTempted");
    //=Game.Entity.States.InLove = 7
    luaC_setfield_integer(L, 7, "InLove");
    //=Game.Entity.States.HasSaddle = 8
    luaC_setfield_integer(L, 8, "HasSaddle");
    //=Game.Entity.States.IsAdult = 11
    luaC_setfield_integer(L, 11, "IsAdult");
    //=Game.Entity.States.Named = 14
    luaC_setfield_integer(L, 14, "Named");
    //=Game.Entity.States.IsTamed = 27
    luaC_setfield_integer(L, 27, "IsTamed");
    //=Game.Entity.States.Leaded = 28
    luaC_setfield_integer(L, 28, "Leaded");
    //=Game.Entity.States.Sheared = 29
    luaC_setfield_integer(L, 29, "Sheared");
    //=Game.Entity.States.ElytraFly = 30
    luaC_setfield_integer(L, 30, "ElytraFly");
    lua_setfield(L, -2, "States");

    lua_setfield(L, -2, "Entity");
    lua_pop(L, 1);
    return true;
}