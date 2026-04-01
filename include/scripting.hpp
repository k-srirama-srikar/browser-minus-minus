#pragma once

#include <string>

struct Node;

bool initScripting(Node* documentRoot);
void shutdownScripting();
void runScript(const std::string& code);
