#pragma once
#include <string>

namespace Utils {
    // Removes trailing commas from a JSON string to make it valid for nlohmann::json
    std::string stripTrailingCommas(const std::string& json);
}
