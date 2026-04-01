#define SOL_ALL_SAFETIES_ON 1
#include "scripting.hpp"
#include "dom.hpp"
#include <sol/sol.hpp>
#include <iostream>
#include <vector>
#include <chrono>
#include <map>

static sol::state* g_lua = nullptr;
static Node* g_document = nullptr;
static std::map<int, sol::function> g_clickHandlers;

static Node* findNodeByIdRecursive(Node* current, int id) {
    if (!current) return nullptr;
    if (current->id == id) return current;
    for (Node* child : current->children) {
        Node* found = findNodeByIdRecursive(child, id);
        if (found) return found;
    }
    return nullptr;
}

static void findNodesByTagRecursive(Node* current, const std::string& tag, std::vector<int>& outIds) {
    if (!current) return;
    if (current->tag == tag) {
        outIds.push_back(current->id);
    }
    for (Node* child : current->children) {
        findNodesByTagRecursive(child, tag, outIds);
    }
}

static sol::object jsonToLua(const nlohmann::json& j, sol::state_view lua) {
    if (j.is_null()) return sol::make_object(lua, sol::nil);
    if (j.is_boolean()) return sol::make_object(lua, j.get<bool>());
    if (j.is_number_integer()) return sol::make_object(lua, j.get<int>());
    if (j.is_number_float()) return sol::make_object(lua, j.get<float>());
    if (j.is_string()) return sol::make_object(lua, j.get<std::string>());
    if (j.is_array()) {
        sol::table t = lua.create_table();
        for (size_t i = 0; i < j.size(); ++i) {
            t[i + 1] = jsonToLua(j[i], lua);
        }
        return t;
    }
    if (j.is_object()) {
        sol::table t = lua.create_table();
        for (auto& [k, v] : j.items()) {
            t[k] = jsonToLua(v, lua);
        }
        return t;
    }
    return sol::make_object(lua, sol::nil);
}

static nlohmann::json luaToJson(const sol::object& obj) {
    if (obj.is<bool>()) return obj.as<bool>();
    if (obj.is<int>()) return obj.as<int>();
    if (obj.is<float>()) return obj.as<float>();
    if (obj.is<std::string>()) return obj.as<std::string>();
    if (obj.is<sol::table>()) {
        sol::table t = obj.as<sol::table>();
        bool isArray = true;
        int expectedKey = 1;
        for (const auto& kv : t) {
            if (!kv.first.is<int>() || kv.first.as<int>() != expectedKey++) {
                isArray = false;
                break;
            }
        }
        if (isArray && !t.empty()) {
            nlohmann::json arr = nlohmann::json::array();
            for (const auto& kv : t) {
                arr.push_back(luaToJson(kv.second));
            }
            return arr;
        } else {
            nlohmann::json ret = nlohmann::json::object();
            for (const auto& kv : t) {
                if (kv.first.is<std::string>()) {
                    ret[kv.first.as<std::string>()] = luaToJson(kv.second);
                }
            }
            return ret;
        }
    }
    return nlohmann::json();
}

bool initScripting(Node* documentRoot) {
    g_document = documentRoot;
    g_lua = new sol::state();
    if (!g_lua) return false;
    g_lua->open_libraries(sol::lib::base, sol::lib::package, sol::lib::string, sol::lib::math, sol::lib::table, sol::lib::os);

    sol::table browser = g_lua->create_table("browser");

    browser.set_function("log", [](const std::string& msg) {
        std::cout << "[Lua Log] " << msg << std::endl;
    });

    browser.set_function("getTime", []() -> double {
        return static_cast<double>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count());
    });

    browser.set_function("getElemsByTag", [](const std::string& tag) {
        std::vector<int> ids;
        findNodesByTagRecursive(g_document, tag, ids);
        sol::table result = g_lua->create_table();
        for (size_t i = 0; i < ids.size(); ++i) {
            result[i + 1] = ids[i];
        }
        return result;
    });

    browser.set_function("getElem", [](int id) -> sol::object {
        Node* node = findNodeByIdRecursive(g_document, id);
        if (!node) return sol::nil;
        return jsonToLua(node->properties, *g_lua);
    });

    browser.set_function("updateElem", [](int id, sol::object properties) {
        Node* node = findNodeByIdRecursive(g_document, id);
        if (!node) return;
        node->properties = luaToJson(properties);
    });

    browser.set_function("addClickHandler", [](int id, sol::function cb) {
        g_clickHandlers[id] = cb;
        std::cout << "[Lua] click handler attached for element " << id << std::endl;
    });

    return true;
}

void triggerScriptClick(int id) {
    if (g_clickHandlers.find(id) != g_clickHandlers.end()) {
        try {
            g_clickHandlers[id]();
        } catch (const sol::error& e) {
            std::cerr << "Lua click handle error: " << e.what() << std::endl;
        }
    }
}

void shutdownScripting() {
    if (g_lua) {
        delete g_lua;
        g_lua = nullptr;
    }
}

void runScript(const std::string& code) {
    if (!g_lua || code.empty()) return;
    try {
        g_lua->script(code);
    } catch (const sol::error& e) {
        std::cerr << "Lua script eval error: " << e.what() << std::endl;
    }
}
