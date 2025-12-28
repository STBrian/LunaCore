#include "Core/GameModules/Recipes.hpp"

#include <string>
#include <vector>

#include <CTRPluginFramework.hpp>

#include "lua_object.hpp"

#include "Core/Event.hpp"
#include "Core/Debug.hpp"

#include "game/world/item/Recipes.hpp"

#include "lua_utils.hpp"

namespace CTRPF = CTRPluginFramework;

static bool repeatedValue(std::vector<Minecraft::RecipeComponentDefIns>& components, char id) {
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

/* Consumes the table in stack. Returns NULL on success, errorMsg if fail */
static const char* ProcessTableKeys(lua_State* L, std::vector<Minecraft::RecipeComponentDefIns>& components) {
    int top = lua_gettop(L) - 1;
    lua_pushnil(L);
    while (lua_next(L, -2) != 0) {
        char idc;
        Minecraft::ItemInstance* item = nullptr;
        // Must be a table of tables with two elements
        if (!lua_istable(L, -1)) {
            lua_settop(L, top); // restore stack and remove table
            return "Unexpected type in components table (expected table)";
        } else {
            lua_rawgeti(L, -1, 1);
            // First element must be the id
            if (!lua_isstring(L, -1)) {
                lua_settop(L, top); // restore stack and remove table
                return "Unexpected type in component id (expected string)";
            } else {
                std::string id(lua_tostring(L, -1));
                if (id.empty()) {
                    lua_settop(L, top); // restore stack and remove table
                    return "Unexpected type in component id (expected non empty string)";
                }
                idc = id[0];
                if (repeatedValue(components, idc)) {
                    lua_settop(L, top); // restore stack and remove table
                    return "Repeated key";
                }
            }
            lua_pop(L, 1);

            // Second element must be the GameItemInstance
            lua_rawgeti(L, -1, 2);
            if (!LuaObject::IsObject(L, -1, "GameItemInstance")) {
                lua_settop(L, top); // restore stack and remove table
                return "Unexpected type in component item (expected GameItemInstance)";
            } else {
                item = *(Minecraft::ItemInstance**)lua_touserdata(L, -1);
            }
            lua_pop(L, 1);
        }

        if (item != nullptr) {
            components.push_back({idc, item});
        }

        lua_pop(L, 1);
    }
    lua_settop(L, top); // restore stack and remove table
    return nullptr;
}

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
### Game.Recipes.registerShapedRecipe
*/
static int l_Recipes_registerShapedRecipe(lua_State* L) {
    void* ptr1 = *(void**)LuaObject::CheckObject(L, 1, "RecipesTable");
    Minecraft::ItemInstance* resultItem = *(Minecraft::ItemInstance**)LuaObject::CheckObject(L, 2, "GameItemInstance");
    u16 categoryId = luaL_checkinteger(L, 3);
    s16 position = luaL_checkinteger(L, 4);
    size_t line1s = 0, line2s = 0, line3s = 0;
    const char* line1 = luaL_checklstring(L, 5, &line1s);
    const char* line2 = luaL_checklstring(L, 6, &line2s);
    const char* line3 = luaL_checklstring(L, 7, &line3s);
    luaL_checktype(L, 8, LUA_TTABLE); // Table
    if (line1s > 3 || line2s > 3 || line3s > 3)
        return luaL_error(L, "shape is not a square!");
    if (line1s + line2s + line3s < 1)
        return luaL_error(L, "shape is empty!");

    {
    std::vector<Minecraft::RecipeComponentDefIns> components;
    lua_pushvalue(L, 8); // Table
    const char* errorMsg = ProcessTableKeys(L, components);

    if (errorMsg)
        LUAUTILS_ERRORF(L, errorMsg);

    if (components.empty())
        LUAUTILS_ERRORF(L, "no components were passed!");

    gstd::vector<Minecraft::InternalRecipeElementDefinition> vec;
    Minecraft::definition(vec, components.data(), components.size());
    if (line3s == 0 && line2s == 0 && line1s < 3 ) {
        Minecraft::Recipes::addShapedRecipe(ptr1, *resultItem, line1, vec, categoryId, position);
    } else if (line3s == 0 && line1s < 3 && line2s < 3) {
        if (line1s == 0) line1 = " ";
        if (line2s == 0) line2 = " ";
        Minecraft::Recipes::addShapedRecipe(ptr1, *resultItem, line1, line2, vec, categoryId, position);
    } else {
        if (line1s == 0) line1 = " ";
        if (line2s == 0) line2 = " ";
        if (line3s == 0) line3 = " ";
        Minecraft::Recipes::Shape shape = {line1, line2, line3};
        Minecraft::Recipes::addShapedRecipe(ptr1, *resultItem, shape, vec, categoryId, position);
    }
    return 0;
    }

    LUAUTILS_SET_ERROR_HANDLER(L); // "Safe" c++ error handler
}

static const luaL_Reg recipes_functions[] = {
    {"registerShapedRecipe", l_Recipes_registerShapedRecipe},
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