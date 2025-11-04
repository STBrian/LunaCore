#include "Core/Game/Recipes.hpp"

#include <string>
#include <vector>

#include <CTRPluginFramework.hpp>

#include "lua_object.hpp"

#include "Core/Event.hpp"
#include "Core/Debug.hpp"

#include "Minecraft/world/item/Recipes.hpp"

namespace CTRPF = CTRPluginFramework;

static bool repeatedValue(std::vector<Minecraft::RecipeComponentDef>& components, char id) {
    bool repeated = false;
    for (auto& component : components) {
        if (id == component.id)
            return true;
    }
    return repeated;
}

// ----------------------------------------------------------------------------

//$Game.Recipes
//@@RecipesTable

// ----------------------------------------------------------------------------

/*
- Allows to register a recipe. Use with the value given in event Game.Recipes.OnRegisterRecipes to get RecipesTable value
## recipesTable: RecipesTable
## resultItem: GameItemInstance
## categoryId: integer
## position: integer
## line1: string
## line2: string
## line3: string
## components: table
### Game.Recipes.registerRecipe
*/
static int l_Recipes_registerRecipe(lua_State* L) {
    void* ptr1 = *(void**)LuaObject::CheckObject(L, 1, "RecipesTable");
    Minecraft::ItemInstance* resultItem = *(Minecraft::ItemInstance**)LuaObject::CheckObject(L, 2, "GameItemInstance");
    u16 categoryId = luaL_checkinteger(L, 3);
    s16 position = luaL_checkinteger(L, 4);
    std::string line1 = luaL_checkstring(L, 5);
    std::string line2 = luaL_checkstring(L, 6);
    std::string line3 = luaL_checkstring(L, 7);
    luaL_checktype(L, 8, LUA_TTABLE); // Table
    if (line1.length() > 3 || line2.length() > 3 || line3.length() > 3)
        return luaL_error(L, "shape is not a square!");
    if (line1.length() + line2.length() + line3.length() < 1)
        return luaL_error(L, "shape is empty!");

    std::vector<Minecraft::RecipeComponentDefIns> components;
    lua_pushvalue(L, 8); // Table
    lua_pushnil(L);
    while (lua_next(L, -2) != 0) {
        char idc;
        Minecraft::ItemInstance* item = nullptr;
        // Must be a table of tables with two elements
        if (!lua_istable(L, -1)) {
            lua_pop(L, 3);
            return luaL_error(L, "Unexpected type in components table (expected table)");
        } else {
            lua_rawgeti(L, -1, 1);
            // First element must be the id
            if (!lua_isstring(L, -1)) {
                lua_pop(L, 4);
                return luaL_error(L, "Unexpected type in component id (expected string)");
            } else {
                std::string id(lua_tostring(L, -1));
                if (id.empty()) {
                    lua_pop(L, 4);
                    return luaL_error(L, "Unexpected type in component id (expected non empty string)");
                }
                idc = id[0];
            }
            lua_pop(L, 1);

            // Second element must be the GameItemInstance
            lua_rawgeti(L, -1, 2);
            if (!LuaObject::IsObject(L, -1, "GameItemInstance")) {
                lua_pop(L, 4);
                return luaL_error(L, "Unexpected type in component item (expected GameItemInstance)");
            } else {
                item = *(Minecraft::ItemInstance**)lua_touserdata(L, -1);
            }
            lua_pop(L, 1);
        }

        // Get the block if the id is from a block
        if (item != nullptr) {
            components.push_back({idc, item});
        }

        lua_pop(L, 1);
    }
    lua_pop(L, 1);

    if (components.empty())
        return luaL_error(L, "no components were passed!");

    Minecraft::Recipes::Shape shape = {line1, line2, line3};
    GenericVectorType<Minecraft::InternalRecipeElementDefinition> vec;
    Minecraft::definition(vec, components.data(), components.size());
    Minecraft::Recipes::addShapedRecipe(ptr1, *resultItem, shape, vec, categoryId, position);
    return 0;
}

static const luaL_Reg recipes_functions[] = {
    {"registerRecipe", l_Recipes_registerRecipe},
    {NULL, NULL}
};

static const LuaObjectField RecipesTableFields[] = {
    {"dummy", OBJF_TYPE_INT, 0},
    {NULL, OBJF_TYPE_NIL, 0}
};

bool Core::Module::RegisterRecipesModule(lua_State *L) {
    LuaObject::RegisterNewObject(L, "RecipesTable", RecipesTableFields);

    lua_getglobal(L, "Game");
    lua_newtable(L); // Recipes

    //$@@@Game.Recipes.OnRegisterRecipes: EventClass
    Core::Event::NewEvent(L, "OnRegisterRecipes");

    luaL_register(L, NULL, recipes_functions);

    lua_setfield(L, -2, "Recipes");
    lua_pop(L, 1);
    return true;
}