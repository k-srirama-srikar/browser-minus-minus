#pragma once

#include <string>

struct Node;

bool initScripting(Node* documentRoot);
void shutdownScripting();
void runScript(const std::string& code);
void triggerScriptClick(int id);
void setCurrentUrl(const std::string& url);
void setScreenSize(int width, int height);
