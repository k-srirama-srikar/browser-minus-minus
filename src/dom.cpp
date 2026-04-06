#include "dom.hpp"
#include <iostream>

using json = nlohmann::json;

static int nextNodeId() {
    static int id = 1;
    return id++;
}

static Node* parseNodeRecursive(const json& jElement) {
    if (!jElement.is_object()) return nullptr;
    
    Node* node = new Node();
    node->id = nextNodeId();
    node->type = NodeType::FlexV; // Default type

    if (jElement.contains("tag")) {
        if (jElement["tag"].is_string()) {
            node->tag = jElement["tag"].get<std::string>();
        } else if (jElement["tag"].is_number()) {
            node->tag = std::to_string(jElement["tag"].get<int>());
        }
    }

    nlohmann::json props = nlohmann::json::object();
    for (auto& [key, val] : jElement.items()) {
        if (key != "tag") {
            props[key] = val;
        }
    }

    // Determine type but don't make it exclusive for properties
    if (jElement.contains("flexv") && jElement["flexv"].is_array()) {
        node->type = NodeType::FlexV;
        for (const auto& childJson : jElement["flexv"]) {
            Node* child = parseNodeRecursive(childJson);
            if (child) node->children.push_back(child);
        }
    } else if (jElement.contains("flexh") && jElement["flexh"].is_array()) {
        node->type = NodeType::FlexH;
        for (const auto& childJson : jElement["flexh"]) {
            Node* child = parseNodeRecursive(childJson);
            if (child) node->children.push_back(child);
        }
    } else if (jElement.contains("text")) {
        node->type = NodeType::Text;
    } else if (jElement.contains("image")) {
        node->type = NodeType::Image;
    }

    node->properties = props;
    return node;
}

Node* parseMarkup(const std::string& source) {
    try {
        json j = json::parse(source, nullptr, true, true);
        
        Node* root = new Node();
        root->id = nextNodeId();
        root->type = NodeType::Root;
        root->tag = "root_document";
        root->properties = nlohmann::json::object();
        
        if (j.contains("lua") && j["lua"].is_string()) {
            root->properties["lua"] = j["lua"].get<std::string>();
        }

        if (j.contains("root") && j["root"].is_object()) {
            Node* child = parseNodeRecursive(j["root"]);
            if (child) {
                root->children.push_back(child);
            }
        }
        return root;
    } catch (const json::parse_error& e) {
        std::cerr << "JSML JSON Parse Error: " << e.what() << std::endl;
        return nullptr;
    }
}

void destroyDom(Node* root) {
    if (!root) return;
    for (Node* child : root->children) {
        destroyDom(child);
    }
    delete root;
}
