#include "network.hpp"
#include <curl/curl.h>
#include <fstream>
#include <sstream>

static size_t curlWriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t total = size * nmemb;
    std::string* buffer = static_cast<std::string*>(userp);
    buffer->append(static_cast<char*>(contents), total);
    return total;
}

bool fetchUrl(const std::string& url, std::string& output) {
    CURL* handle = curl_easy_init();
    if (!handle) {
        return false;
    }

    curl_easy_setopt(handle, CURLOPT_URL, url.c_str());
    curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, curlWriteCallback);
    curl_easy_setopt(handle, CURLOPT_WRITEDATA, &output);
    CURLcode result = curl_easy_perform(handle);
    curl_easy_cleanup(handle);
    return result == CURLE_OK;
}

bool loadFile(const std::string& path, std::string& output) {
    std::ifstream file(path);
    if (!file) {
        return false;
    }
    std::ostringstream buffer;
    buffer << file.rdbuf();
    output = buffer.str();
    return true;
}
