#pragma once

#include <string>
#include <memory>

struct Node;
class Tab;
class TabManager;

// Legacy global scripting API (no longer recommended)
bool initScripting(Node* documentRoot);
void shutdownScripting();
void runScript(const std::string& code);
void triggerScriptClick(int id);
void setCurrentUrl(const std::string& url);
void setScreenSize(int width, int height);

// Tab-aware scripting API (recommended)
// This allows each tab to have its own isolated Lua environment
bool initTabScripting(Tab* tab, int screenWidth, int screenHeight);
void shutdownTabScripting(Tab* tab);
void runTabScript(Tab* tab, const std::string& code);
void triggerTabScriptClick(Tab* tab, int nodeId);
void setTabScreenSize(Tab* tab, int width, int height);
