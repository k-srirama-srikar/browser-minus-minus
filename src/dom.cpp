#include "dom.hpp"
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <sstream>

static int nextNodeId() {
    static int id = 1;
    return id++;
}

static std::string trim(const std::string& value) {
    size_t left = 0;
    size_t right = value.size();
    while (left < right && std::isspace(static_cast<unsigned char>(value[left]))) {
        left++;
    }
    while (right > left && std::isspace(static_cast<unsigned char>(value[right - 1]))) {
        right--;
    }
    return value.substr(left, right - left);
}

static std::string lowerCase(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return text;
}

static void parseAttributes(const std::string& text, std::map<std::string, std::string>& attrs) {
    size_t position = 0;
    while (position < text.size()) {
        while (position < text.size() && std::isspace(static_cast<unsigned char>(text[position]))) {
            position++;
        }
        if (position >= text.size()) {
            break;
        }

        size_t keyStart = position;
        while (position < text.size() && text[position] != '=' && !std::isspace(static_cast<unsigned char>(text[position]))) {
            position++;
        }
        std::string key = lowerCase(trim(text.substr(keyStart, position - keyStart)));

        while (position < text.size() && std::isspace(static_cast<unsigned char>(text[position]))) {
            position++;
        }
        if (position >= text.size() || text[position] != '=') {
            break;
        }
        position++;

        while (position < text.size() && std::isspace(static_cast<unsigned char>(text[position]))) {
            position++;
        }

        char quote = 0;
        if (position < text.size() && (text[position] == '"' || text[position] == '\'')) {
            quote = text[position++];
        }

        size_t valueStart = position;
        std::string value;
        if (quote) {
            while (position < text.size() && text[position] != quote) {
                position++;
            }
            value = text.substr(valueStart, position - valueStart);
            if (position < text.size() && text[position] == quote) {
                position++;
            }
        } else {
            while (position < text.size() && !std::isspace(static_cast<unsigned char>(text[position]))) {
                position++;
            }
            value = text.substr(valueStart, position - valueStart);
        }

        if (!key.empty()) {
            attrs[key] = trim(value);
        }
    }
}

static bool isWhitespaceOnly(const std::string& text) {
    for (char ch : text) {
        if (!std::isspace(static_cast<unsigned char>(ch))) {
            return false;
        }
    }
    return true;
}

static NodeType nodeTypeFromTag(const std::string& tag, const std::map<std::string, std::string>& attrs) {
    std::string loweredStyle;
    auto styleIt = attrs.find("style");
    if (styleIt != attrs.end()) {
        loweredStyle = lowerCase(styleIt->second);
    }

    if (tag == "img" || tag == "image") {
        return NodeType::Image;
    }
    if (tag == "flexh" || tag == "row") {
        return NodeType::FlexH;
    }
    if (tag == "flexv" || tag == "column") {
        return NodeType::FlexV;
    }
    if (loweredStyle.find("flex-direction:row") != std::string::npos) {
        return NodeType::FlexH;
    }
    if (loweredStyle.find("flex-direction:column") != std::string::npos) {
        return NodeType::FlexV;
    }
    if (loweredStyle.find("display:flex") != std::string::npos) {
        return NodeType::FlexV;
    }

    return NodeType::FlexV;
}

static Node* createNode(NodeType type) {
    Node* node = new Node();
    node->id = nextNodeId();
    node->type = type;
    return node;
}

Node* parseMarkup(const std::string& source) {
    Node* root = createNode(NodeType::FlexV);
    root->attrs["style"] = "padding:16;gap:10";
    root->attrs["tag"] = "root";
    std::vector<Node*> stack;
    stack.push_back(root);
    size_t pos = 0;
    const size_t length = source.size();

    while (pos < length) {
        if (source[pos] == '<') {
            if (pos + 1 < length && source[pos + 1] == '/') {
                size_t closeTagEnd = source.find('>', pos + 2);
                if (closeTagEnd == std::string::npos) {
                    break;
                }
                if (stack.size() > 1) {
                    stack.pop_back();
                }
                pos = closeTagEnd + 1;
                continue;
            }

            size_t tagEnd = source.find('>', pos + 1);
            if (tagEnd == std::string::npos) {
                break;
            }
            bool selfClosing = false;
            size_t contentEnd = tagEnd;
            if (tagEnd > pos + 1 && source[tagEnd - 1] == '/') {
                selfClosing = true;
                contentEnd = tagEnd - 1;
            }

            std::string inside = source.substr(pos + 1, contentEnd - pos - 1);
            std::string tagName;
            size_t index = 0;
            while (index < inside.size() && !std::isspace(static_cast<unsigned char>(inside[index]))) {
                tagName.push_back(inside[index]);
                index++;
            }
            tagName = lowerCase(trim(tagName));
            std::string attributeText = inside.substr(index);
            std::map<std::string, std::string> attrs;
            parseAttributes(attributeText, attrs);

            NodeType type = nodeTypeFromTag(tagName, attrs);
            Node* node = createNode(type);
            node->attrs = attrs;
            node->attrs["tag"] = tagName;

            if (tagName == "script" && !selfClosing) {
                size_t endTag = source.find("</script>", tagEnd + 1);
                if (endTag != std::string::npos) {
                    std::string scriptText = source.substr(tagEnd + 1, endTag - (tagEnd + 1));
                    node->attrs["script"] = scriptText;
                    stack.back()->children.push_back(node);
                    pos = endTag + 9;
                    continue;
                }
            }

            stack.back()->children.push_back(node);
            if (!selfClosing && type != NodeType::Image && tagName != "script") {
                stack.push_back(node);
            }
            pos = tagEnd + 1;
            continue;
        }

        size_t nextTag = source.find('<', pos);
        std::string text = source.substr(pos, nextTag == std::string::npos ? length - pos : nextTag - pos);
        if (!isWhitespaceOnly(text)) {
            Node* textNode = createNode(NodeType::Text);
            textNode->content = trim(text);
            textNode->attrs["tag"] = "text";
            stack.back()->children.push_back(textNode);
        }
        if (nextTag == std::string::npos) {
            break;
        }
        pos = nextTag;
    }

    return root;
}

void destroyDom(Node* root) {
    if (!root) {
        return;
    }
    for (Node* child : root->children) {
        destroyDom(child);
    }
    delete root;
}
