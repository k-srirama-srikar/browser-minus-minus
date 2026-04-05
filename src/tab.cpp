#include "tab.hpp"
#include "network.hpp"
#include "layout.hpp"
#include <iostream>
#include <chrono>

// ============================================================================
// DOM Traversal Utilities (private to Tab)
// ============================================================================

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

// = // ============================================================================
// Constants and Helpers
// ============================================================================

static const char* defaultMarkup = R"JSON(
{
  "lua": "",
  "root": {
    "flexv": [
      {
         "text": { "content": "Technitium Toy Browser", "fontsize": 24, "color": "#ffffff" },
         "tag" : "title",
         "bgcolor": "#1a1b26",
         "spacing": 10
      },
      {
         "text": { "content": "Run ./browser assets/index.jsml to see the interactive Lua components.", "fontsize": 14, "color": "#aaaaaa" },
         "tag" : "info",
         "bgcolor": "#24283b",
         "spacing": 5
      }
    ]
  }
}
)JSON";

// ============================================================================
// Tab Implementation
// ============================================================================

Tab::Tab(int id, const std::string& initialUrl)
    : tabId(id), url(initialUrl), domRoot(nullptr) {
    luaState = std::make_unique<sol::state>();
}

Tab::~Tab() {
    clearDom();
    shutdownLua();
}

void Tab::setDomRoot(Node* newRoot) {
    if (domRoot) {
        destroyDom(domRoot);
    }
    domRoot = newRoot;
}

void Tab::clearDom() {
    if (domRoot) {
        destroyDom(domRoot);
        domRoot = nullptr;
    }
}

bool Tab::initializeLua() {
    if (!luaState) {
        return false;
    }
    
    try {
        // Open standard libraries for this tab's isolated Lua state
        luaState->open_libraries(
            sol::lib::base,
            sol::lib::package,
            sol::lib::string,
            sol::lib::math,
            sol::lib::table,
            sol::lib::os
        );
        
        // Create browser API for this tab's Lua environment
        sol::table browser = luaState->create_table("browser");
        
        // Capture 'this' tab pointer for callbacks
        Tab* thisTab = this;
        
        browser.set_function("log", [](const std::string& msg) {
            std::cout << "[Lua Log] " << msg << std::endl;
        });
        
        browser.set_function("getTime", []() -> double {
            return static_cast<double>(
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()
                ).count()
            );
        });
        
        browser.set_function("getCurrentUrl", [thisTab]() {
            return thisTab->url;
        });
        
        browser.set_function("getScreenSize", [thisTab]() -> sol::object {
            sol::table result = thisTab->luaState->create_table();
            result[1] = 1280;  // Default width (TODO: make configurable)
            result[2] = 720;   // Default height (TODO: make configurable)
            return result;
        });
        
        browser.set_function("getElemRect", [thisTab](int id) -> sol::object {
            if (!thisTab->domRoot) {
                return sol::nil;
            }
            
            Node* node = findNodeByIdRecursive(thisTab->domRoot, id);
            if (!node) {
                return sol::nil;
            }
            
            sol::table rect = thisTab->luaState->create_table();
            rect[1] = node->x;
            rect[2] = node->y;
            rect[3] = node->width;
            rect[4] = node->height;
            return rect;
        });
        
        browser.set_function("getElem", [thisTab](int id) -> sol::object {
            if (!thisTab->domRoot) return sol::nil;
            
            Node* node = findNodeByIdRecursive(thisTab->domRoot, id);
            if (!node) return sol::nil;
            
            // Convert node with all properties to Lua table
            sol::table elemTable = thisTab->luaState->create_table();
            elemTable["id"] = node->id;
            elemTable["tag"] = node->tag;
            elemTable["x"] = node->x;
            elemTable["y"] = node->y;
            elemTable["width"] = node->width;
            elemTable["height"] = node->height;
            
            // Copy all properties from the node's JSON
            for (auto& [key, value] : node->properties.items()) {
                if (value.is_object()) {
                    // Recursively convert nested objects
                    sol::table nestedTable = thisTab->luaState->create_table();
                    for (auto& [subkey, subval] : value.items()) {
                        if (subval.is_string()) {
                            nestedTable[subkey] = subval.get<std::string>();
                        } else if (subval.is_number_integer()) {
                            nestedTable[subkey] = subval.get<int>();
                        } else if (subval.is_number_float()) {
                            nestedTable[subkey] = subval.get<float>();
                        } else if (subval.is_boolean()) {
                            nestedTable[subkey] = subval.get<bool>();
                        }
                    }
                    elemTable[key] = nestedTable;
                } else if (value.is_string()) {
                    elemTable[key] = value.get<std::string>();
                } else if (value.is_number_integer()) {
                    elemTable[key] = value.get<int>();
                } else if (value.is_number_float()) {
                    elemTable[key] = value.get<float>();
                } else if (value.is_boolean()) {
                    elemTable[key] = value.get<bool>();
                }
            }
            
            return elemTable;
        });
        
        browser.set_function("getElemsByTag", [thisTab](const std::string& tag) -> sol::object {
            if (!thisTab->domRoot) {
                return thisTab->luaState->create_table();
            }
            
            std::vector<int> ids;
            findNodesByTagRecursive(thisTab->domRoot, tag, ids);
            
            sol::table result = thisTab->luaState->create_table();
            for (size_t i = 0; i < ids.size(); ++i) {
                result[i + 1] = ids[i];
            }
            return result;
        });
        
        browser.set_function("setElemText", [thisTab](int id, const std::string& text) {
            if (!thisTab->domRoot) return;
            
            Node* node = findNodeByIdRecursive(thisTab->domRoot, id);
            if (node) {
                if (node->properties.contains("text")) {
                    if (node->properties["text"].is_object()) {
                        node->properties["text"]["content"] = text;
                    }
                } else {
                    node->properties["text"] = {{"content", text}};
                }
            }
        });
        
        browser.set_function("setElemProp", [thisTab](int id, const std::string& prop, const sol::object& value) {
            if (!thisTab->domRoot) return;
            
            Node* node = findNodeByIdRecursive(thisTab->domRoot, id);
            if (!node) return;
            
            // Convert sol::object to JSON value
            if (value.is<bool>()) {
                node->properties[prop] = value.as<bool>();
            } else if (value.is<int>()) {
                node->properties[prop] = value.as<int>();
            } else if (value.is<float>()) {
                node->properties[prop] = value.as<float>();
            } else if (value.is<std::string>()) {
                node->properties[prop] = value.as<std::string>();
            }
        });
        
        browser.set_function("fetch", [thisTab](const std::string& urlStr, sol::function callback) {
            std::string response;
            bool success = false;
            
            if (urlStr.rfind("http://", 0) == 0 || urlStr.rfind("https://", 0) == 0) {
                success = fetchUrl(urlStr, response);
            } else {
                success = loadFile(urlStr, response);
            }
            
            sol::object arg;
            if (success) {
                try {
                    arg = sol::make_object(*thisTab->luaState, response);
                } catch (...) {
                    arg = sol::make_object(*thisTab->luaState, sol::nil);
                }
            } else {
                arg = sol::make_object(*thisTab->luaState, sol::nil);
            }
            
            try {
                callback(arg);
            } catch (const sol::error& e) {
                std::cerr << "Lua fetch callback error: " << e.what() << std::endl;
            }
        });
        
        browser.set_function("on", [thisTab](int nodeId, const std::string& eventType, sol::function handler) {
            if (eventType == "click") {
                thisTab->registerClickHandler(nodeId, handler);
            }
        });
        
        // addClickHandler - alias for browser.on("click") per Lua runtime spec
        browser.set_function("addClickHandler", [thisTab](int nodeId, sol::function handler) {
            thisTab->registerClickHandler(nodeId, handler);
        });
        
        // updateElem - update element properties in the DOM tree
        browser.set_function("updateElem", [thisTab](int id, sol::table elemTable) {
            if (!thisTab->domRoot) return;
            
            Node* node = findNodeByIdRecursive(thisTab->domRoot, id);
            if (!node) return;
            
            // Iterate through the Lua table and update node properties
            for (auto& [key, value] : elemTable) {
                // Skip structural properties
                if (key.as<std::string>() == "id" || 
                    key.as<std::string>() == "tag" ||
                    key.as<std::string>() == "x" ||
                    key.as<std::string>() == "y" ||
                    key.as<std::string>() == "width" ||
                    key.as<std::string>() == "height") {
                    continue;
                }
                
                // Convert sol::object to JSON and store
                if (value.is<bool>()) {
                    node->properties[key.as<std::string>()] = value.as<bool>();
                } else if (value.is<int>()) {
                    node->properties[key.as<std::string>()] = value.as<int>();
                } else if (value.is<float>()) {
                    node->properties[key.as<std::string>()] = value.as<float>();
                } else if (value.is<std::string>()) {
                    node->properties[key.as<std::string>()] = value.as<std::string>();
                } else if (value.is<sol::table>()) {
                    // Handle nested tables (like "text" with "content" field)
                    sol::table nestedTable = value.as<sol::table>();
                    nlohmann::json nestedJson = nlohmann::json::object();
                    for (auto& [subkey, subval] : nestedTable) {
                        std::string fieldName = subkey.as<std::string>();
                        if (subval.is<bool>()) {
                            nestedJson[fieldName] = subval.as<bool>();
                        } else if (subval.is<int>()) {
                            nestedJson[fieldName] = subval.as<int>();
                        } else if (subval.is<float>()) {
                            nestedJson[fieldName] = subval.as<float>();
                        } else if (subval.is<std::string>()) {
                            nestedJson[fieldName] = subval.as<std::string>();
                        }
                    }
                    node->properties[key.as<std::string>()] = nestedJson;
                }
            }
        });
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to initialize Lua for tab " << tabId << ": " << e.what() << std::endl;
        return false;
    }
}

void Tab::shutdownLua() {
    clickHandlers.clear();
    if (luaState) {
        luaState.reset();
    }
}

void Tab::runScript(const std::string& code) {
    if (!luaState) {
        return;
    }
    
    try {
        luaState->safe_script(code);
    } catch (const sol::error& e) {
        std::cerr << "Lua error in tab " << tabId << ": " << e.what() << std::endl;
    }
}

void Tab::triggerClick(int nodeId) {
    sol::function* handler = getClickHandler(nodeId);
    if (handler && handler->valid()) {
        try {
            (*handler)(nodeId);
        } catch (const sol::error& e) {
            std::cerr << "Click handler error in tab " << tabId << ": " << e.what() << std::endl;
        }
    }
}

// ============================================================================
// TabManager Implementation
// ============================================================================

TabManager::TabManager() : activeTabId(-1) {
}

TabManager::~TabManager() {
    closeAllTabs();
}

Tab* TabManager::createTab(const std::string& url) {
    if (tabs.size() >= MAX_TABS) {
        std::cerr << "Maximum number of tabs reached (" << MAX_TABS << ")" << std::endl;
        return nullptr;
    }
    
    int newTabId = nextTabId++;
    auto newTab = std::make_unique<Tab>(newTabId, url);
    
    if (!newTab->initializeLua()) {
        std::cerr << "Failed to initialize Lua environment for tab " << newTabId << std::endl;
        return nullptr;
    }
    
    Tab* tabPtr = newTab.get();
    tabs.push_back(std::move(newTab));
    
    // Activate the first tab automatically
    if (activeTabId == -1) {
        activeTabId = newTabId;
    }
    
    return tabPtr;
}

bool TabManager::closeTab(int tabId) {
    auto it = std::find_if(
        tabs.begin(),
        tabs.end(),
        [tabId](const std::unique_ptr<Tab>& tab) { return tab->getId() == tabId; }
    );
    
    if (it == tabs.end()) {
        return false;
    }
    
    tabs.erase(it);
    
    // If we closed the active tab, switch to another
    if (activeTabId == tabId) {
        if (!tabs.empty()) {
            activeTabId = tabs[0]->getId();
        } else {
            activeTabId = -1;
        }
    }
    
    return true;
}

Tab* TabManager::getTab(int tabId) {
    for (auto& tab : tabs) {
        if (tab->getId() == tabId) {
            return tab.get();
        }
    }
    return nullptr;
}

Tab* TabManager::getActiveTab() {
    return getTab(activeTabId);
}

bool TabManager::switchTab(int tabId) {
    if (getTab(tabId) == nullptr) {
        return false;
    }
    activeTabId = tabId;
    return true;
}

std::vector<int> TabManager::getAllTabIds() const {
    std::vector<int> ids;
    for (const auto& tab : tabs) {
        ids.push_back(tab->getId());
    }
    return ids;
}

void TabManager::closeAllTabs() {
    tabs.clear();
    activeTabId = -1;
}

bool TabManager::loadTab(Tab* tab, const std::string& url) {
    if (!tab) return false;
    
    std::string pageSource;
    if (url.rfind("http://", 0) == 0 || url.rfind("https://", 0) == 0) {
        if (!fetchUrl(url, pageSource)) {
            std::cerr << "Failed to fetch URL: " << url << std::endl;
            return false;
        }
    } else {
        if (!loadFile(url, pageSource)) {
            pageSource = defaultMarkup;
        }
    }
    
    Node* newRoot = parseMarkup(pageSource);
    if (!newRoot) {
        std::cerr << "Failed to parse JSML from: " << url << std::endl;
        return false;
    }
    
    tab->setDomRoot(newRoot);
    tab->setUrl(url);
    
    if (!tab->initializeLua()) {
        std::cerr << "Failed to initialize Lua for tab" << std::endl;
        return false;
    }
    
    if (newRoot->properties.contains("lua") && newRoot->properties["lua"].is_string()) {
        std::string scriptPath = newRoot->properties["lua"].get<std::string>();
        if (!scriptPath.empty()) {
            std::string code;
            if (loadFile(scriptPath, code)) {
                tab->runScript(code);
            }
        }
    }
    
    return true;
}

void TabManager::navigateTab(Tab* tab, const std::string& url) {
    if (!tab) return;
    
    std::string pageSource;
    if (url.rfind("http://", 0) == 0 || url.rfind("https://", 0) == 0) {
        if (!fetchUrl(url, pageSource)) {
            std::cerr << "Failed to fetch URL: " << url << std::endl;
            return;
        }
    } else {
        if (!loadFile(url, pageSource)) {
            std::cerr << "Failed to load file: " << url << std::endl;
            return;
        }
    }

    Node* newDoc = parseMarkup(pageSource);
    if (!newDoc) {
        std::cerr << "Failed to parse JSML from: " << url << std::endl;
        return;
    }

    tab->setDomRoot(newDoc);
    tab->setUrl(url);

    tab->shutdownLua();
    if (!tab->initializeLua()) {
        std::cerr << "Lua re-initialization failed." << std::endl;
    }

    if (newDoc->properties.contains("lua") && newDoc->properties["lua"].is_string()) {
        std::string scriptPath = newDoc->properties["lua"].get<std::string>();
        if (!scriptPath.empty()) {
            std::string code;
            if (loadFile(scriptPath, code)) {
                tab->runScript(code);
            }
        }
    }
}
