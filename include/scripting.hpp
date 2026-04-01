#pragma once

#include <string>

bool initScripting();
void shutdownScripting();
void runScript(const std::string& code);
