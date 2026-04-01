#include "scripting.hpp"

extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

#include <iostream>

static lua_State* g_lua = nullptr;

bool initScripting() {
    g_lua = luaL_newstate();
    if (!g_lua) {
        return false;
    }
    luaL_openlibs(g_lua);
    return true;
}

void shutdownScripting() {
    if (g_lua) {
        lua_close(g_lua);
        g_lua = nullptr;
    }
}

void runScript(const std::string& code) {
    if (!g_lua || code.empty()) {
        return;
    }
    int status = luaL_loadstring(g_lua, code.c_str());
    if (status != LUA_OK) {
        std::cerr << "Lua load error: " << lua_tostring(g_lua, -1) << std::endl;
        lua_pop(g_lua, 1);
        return;
    }
    status = lua_pcall(g_lua, 0, 0, 0);
    if (status != LUA_OK) {
        std::cerr << "Lua runtime error: " << lua_tostring(g_lua, -1) << std::endl;
        lua_pop(g_lua, 1);
    }
}
