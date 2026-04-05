#pragma once

#include <string>

namespace UrlUtils {
    // Resolves a relative path against a base URL or base directory.
    // Examples:
    //   base: "http://example.com/page/", rel: "script.lua" -> "http://example.com/page/script.lua"
    //   base: "/home/user/app/", rel: "assets/img.png" -> "/home/user/app/assets/img.png"
    //   base: "http://example.com/", rel: "/css/style.css" -> "http://example.com/css/style.css"
    std::string resolvePath(const std::string& base, const std::string& relative);

    // Extracts the directory/base URL from a full URL or path.
    // Example: "http://example.com/dir/index.jsml" -> "http://example.com/dir/"
    std::string getBaseUrl(const std::string& url);
    
    // Checks if a string starts with a scheme like "http://" or "https://"
    bool isNetworkUrl(const std::string& url);
}
