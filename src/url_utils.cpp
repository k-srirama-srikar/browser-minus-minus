#include "url_utils.hpp"
#include <algorithm>

namespace UrlUtils {

    bool isNetworkUrl(const std::string& url) {
        return (url.rfind("http://", 0) == 0 || url.rfind("https://", 0) == 0);
    }

    std::string getBaseUrl(const std::string& url) {
        if (url.empty()) return "";
        
        // Find the last directory separator
        size_t lastSlash = url.find_last_of("/\\");
        if (lastSlash == std::string::npos) {
            // No slash, maybe it's just a file in current dir
            return "";
        }
        
        return url.substr(0, lastSlash + 1);
    }

    std::string resolvePath(const std::string& base, const std::string& relative) {
        if (relative.empty()) return base;
        
        // If relative is already absolute or has a scheme, return it
        if (isNetworkUrl(relative)) return relative;
        if (relative[0] == '/' || (relative.size() > 1 && relative[1] == ':')) return relative;
        
        if (base.empty()) return relative;
        
        // Ensure base ends with a slash if it looks like a directory
        std::string finalBase = base;
        if (!finalBase.empty() && finalBase.back() != '/' && finalBase.back() != '\\') {
            finalBase += "/";
        }
        
        // Handle ".." and "." in relative path (simplified)
        std::string finalRel = relative;
        if (finalRel.rfind("./", 0) == 0) {
            finalRel = finalRel.substr(2);
        }
        
        return finalBase + finalRel;
    }
}
