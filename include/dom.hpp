#pragma once

#include <map>
#include <string>
#include <vector>

enum class NodeType { FlexV, FlexH, Text, Image };

struct Node {
    int id;
    NodeType type;
    std::map<std::string, std::string> attrs;
    std::string content;
    std::vector<Node*> children;
    float x{};
    float y{};
    float width{};
    float height{};
};

Node* parseMarkup(const std::string& source);
void destroyDom(Node* root);
