#pragma once

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

enum class NodeType { Root, FlexV, FlexH, Text, Image };

struct Node {
    int id;
    NodeType type;
    std::string tag;
    
    // Extensible JSON property bag for styling, text content, alt-text, etc.
    nlohmann::json properties;
    
    std::vector<Node*> children;
    
    // Computed layout constraints
    float x{};
    float y{};
    float width{};
    float height{};
};

Node* parseMarkup(const std::string& source);
void destroyDom(Node* root);
