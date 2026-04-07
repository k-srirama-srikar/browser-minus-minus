#include "utils.hpp"
#include <cctype>

namespace Utils {

std::string stripTrailingCommas(const std::string& input) {
    std::string result;
    result.reserve(input.size());
    
    bool inString = false;
    bool escaped = false;
    
    for (size_t i = 0; i < input.size(); ++i) {
        char c = input[i];
        
        if (inString) {
            result += c;
            if (escaped) {
                escaped = false;
            } else if (c == '\\') {
                escaped = true;
            } else if (c == '"') {
                inString = false;
            }
        } else {
            if (c == '"') {
                inString = true;
                result += c;
            } else if (c == ',') {
                // Look ahead to see if the next non-whitespace character is a closing delimiter
                size_t j = i + 1;
                while (j < input.size() && std::isspace(static_cast<unsigned char>(input[j]))) {
                    j++;
                }
                
                if (j < input.size() && (input[j] == '}' || input[j] == ']')) {
                    // Skip the comma – it's a trailing one
                    continue;
                }
                result += c;
            } else {
                result += c;
            }
        }
    }
    
    return result;
}

}
