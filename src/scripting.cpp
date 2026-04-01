#define SOL_ALL_SAFETIES_ON 1
#include "scripting.hpp"
#include "dom.hpp"
#include <sol/sol.hpp>
#include <iostream>
#include <vector>
#include <chrono>

static sol::state* g_lua = nullptr;
static Node* g_document = nullptr;

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
    auto it = current->attrs.find("tag");
    if (it != current->attrs.end() && it->second == tag) {
        outIds.push_back(current->id);
    }
    for (Node* child : current->children) {
        findNodesByTagRecursive(child, tag, outIds);
    }
}

bool initScripting(Node* documentRoot) {
    g_document = documentRoot;
    g_lua = new sol::state();
    if (!g_lua) {
        return false;
    }
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

    browser.set_function("getElem", [](int id) -> sol::table {
        Node* node = findNodeByIdRecursive(g_document, id);
        if (!node) return sol::nil;
        
        sol::table snap = g_lua->create_table();
        snap["id"] = node->id;
        snap["type"] = static_cast<int>(node->type);
        snap["content"] = node->content;
        
        sol::table attrs = g_lua->create_table();
        for (const auto& pair : node->attrs) {
            attrs[pair.first] = pair.second;
        }
        snap["attrs"] = attrs;
        return snap;
    });

    browser.set_function("updateElem", [](int id, sol::table table) {
        Node* node = findNodeByIdRecursive(g_document, id);
        if (!node) return;
        
        if (table["content"].valid()) {
            node->content = table["content"].get<std::string>();
        }
        if (table["attrs"].valid()) {
            sol::table attrs = table["attrs"];
            for (auto& pair : attrs) {
                if (pair.first.is<std::string>() && pair.second.is<std::string>()) {
                    node->attrs[pair.first.as<std::string>()] = pair.second.as<std::string>();
                }
            }
        }
    });

    browser.set_function("addClickHandler", [](int id, sol::function cb) {
        std::cout << "[Lua] click handler attached for element " << id << std::endl;
    });

    browser.set_function("fetch", [](const std::string& url, sol::function callback) {
        std::cout << "[Lua] fetch pending for: " << url << std::endl;
    });

    return true;
}

void shutdownScripting() {
    if (g_lua) {
        delete g_lua;
        g_lua = nullptr;
    }
}

void runScript(const std::string& code) {
    if (!g_lua || code.empty()) {
        return;
    }
    try {
        g_lua->script(code);
    } catch (const sol::error& e) {
        std::cerr << "Lua script eval error: " << e.what() << std::endl;
    }
}
