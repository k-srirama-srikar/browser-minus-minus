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
        
        // Handle absolute paths for network URLs differently
        if (relative[0] == '/') {
            if (isNetworkUrl(base)) {
                // Resolve against origin (e.g. http://host:port)
                size_t schemeEnd = base.find("://");
                if (schemeEnd != std::string::npos) {
                    size_t hostEnd = base.find('/', schemeEnd + 3);
                    if (hostEnd == std::string::npos) {
                        return base + relative;
                    } else {
                        return base.substr(0, hostEnd) + relative;
                    }
                }
            }
            return relative; // Fallback to original behavior for non-network bases
        }
        
        // Windows-style absolute path
        if (relative.size() > 1 && relative[1] == ':') return relative;
        
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

