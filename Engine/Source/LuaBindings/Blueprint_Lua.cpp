#include "LuaBindings/Blueprint_Lua.h"
#include "LuaBindings/Asset_Lua.h"

#include "LuaBindings/Vector_Lua.h"

#if LUA_ENABLED

void Blueprint_Lua::Bind()
{
    lua_State* L = GetLua();
    int mtIndex = CreateClassMetatable(
        BLUEPRINT_LUA_NAME,
        BLUEPRINT_LUA_FLAG,
        ASSET_LUA_NAME);

    lua_pushcfunction(L, Asset_Lua::Destroy);
    lua_setfield(L, mtIndex, "__gc");

    lua_pop(L, 1);
    OCT_ASSERT(lua_gettop(L) == 0);
}

#endif
