#include "layout.hpp"
#include <algorithm>
#include <cmath>
#include <string>

static float parseFloat(const std::string& text, float fallback) {
    if (text.empty()) {
        return fallback;
    }
    try {
        return std::stof(text);
    } catch (...) {
        return fallback;
    }
}

static std::string lowerCase(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return text;
}

static float parseStyleFloat(const Node* node, const std::string& key, float fallback) {
    auto it = node->attrs.find("style");
    if (it == node->attrs.end()) {
        return fallback;
    }

    std::string style = lowerCase(it->second);
    size_t found = style.find(key + ":");
    if (found == std::string::npos) {
        return fallback;
    }

    size_t start = found + key.size() + 1;
    while (start < style.size() && std::isspace(static_cast<unsigned char>(style[start]))) {
        start++;
    }

    size_t end = start;
    while (end < style.size() && (std::isdigit(static_cast<unsigned char>(style[end])) || style[end] == '.' || style[end] == '-')) {
        end++;
    }

    return parseFloat(style.substr(start, end - start), fallback);
}

static float measureTextWidth(const std::string& text) {
    const float charWidth = 8.0f;
    return std::max(0.0f, static_cast<float>(text.size()) * charWidth);
}

static float measureTextHeight(const std::string& text, float maxWidth) {
    const float charWidth = 8.0f;
    const float lineHeight = 18.0f;
    if (text.empty()) {
        return lineHeight;
    }
    if (maxWidth <= charWidth) {
        return lineHeight * static_cast<float>((text.size() + 1) / 1);
    }
    int columns = std::max(1, static_cast<int>(std::floor(maxWidth / charWidth)));
    int lines = static_cast<int>((text.size() + columns - 1) / columns);
    return std::max(lineHeight, lineHeight * static_cast<float>(lines));
}

void layoutNode(Node* node, float x, float y, float width, float height) {
    if (!node) {
        return;
    }

    node->x = x;
    node->y = y;
    node->width = width;
    node->height = height;

    if (node->type == NodeType::Text) {
        float effectiveWidth = width > 0.0f ? width : measureTextWidth(node->content);
        node->width = effectiveWidth;
        node->height = measureTextHeight(node->content, effectiveWidth);
        return;
    }

    if (node->type == NodeType::Image) {
        float requestedWidth = parseStyleFloat(node, "width", 240.0f);
        float requestedHeight = parseStyleFloat(node, "height", 140.0f);
        node->width = width > 0.0f ? std::min(width, requestedWidth) : requestedWidth;
        node->height = height > 0.0f ? std::min(height, requestedHeight) : requestedHeight;
        return;
    }

    float padding = parseStyleFloat(node, "padding", 12.0f);
    float gap = parseStyleFloat(node, "gap", 8.0f);
    float innerX = x + padding;
    float innerY = y + padding;
    float innerWidth = std::max(0.0f, width - padding * 2.0f);
    float innerHeight = std::max(0.0f, height - padding * 2.0f);
    const int count = static_cast<int>(node->children.size());
    if (count == 0) {
        return;
    }

    float totalGap = gap * static_cast<float>(count - 1);
    if (node->type == NodeType::FlexV) {
        float fixedHeight = 0.0f;
        int flexCount = 0;
        for (Node* child : node->children) {
            if (child->type == NodeType::Text) {
                fixedHeight += measureTextHeight(child->content, innerWidth);
            } else if (child->type == NodeType::Image) {
                fixedHeight += parseStyleFloat(child, "height", 140.0f);
            } else {
                flexCount++;
            }
        }

        float remaining = innerHeight - fixedHeight - totalGap;
        float flexHeight = flexCount > 0 ? std::max(40.0f, remaining / static_cast<float>(flexCount)) : 0.0f;

        float cursorY = innerY;
        for (Node* child : node->children) {
            float childHeight = 0.0f;
            if (child->type == NodeType::Text) {
                childHeight = measureTextHeight(child->content, innerWidth);
            } else if (child->type == NodeType::Image) {
                childHeight = parseStyleFloat(child, "height", 140.0f);
            } else {
                childHeight = std::max(40.0f, flexHeight);
            }
            layoutNode(child, innerX, cursorY, innerWidth, childHeight);
            cursorY += childHeight + gap;
        }
        return;
    }

    if (node->type == NodeType::FlexH) {
        float fixedWidth = 0.0f;
        int flexCount = 0;
        for (Node* child : node->children) {
            if (child->type == NodeType::Text) {
                fixedWidth += measureTextWidth(child->content);
            } else if (child->type == NodeType::Image) {
                fixedWidth += parseStyleFloat(child, "width", 240.0f);
            } else {
                flexCount++;
            }
        }

        float remaining = innerWidth - fixedWidth - totalGap;
        float flexWidth = flexCount > 0 ? std::max(80.0f, remaining / static_cast<float>(flexCount)) : 0.0f;

        float cursorX = innerX;
        for (Node* child : node->children) {
            float childWidth = 0.0f;
            if (child->type == NodeType::Text) {
                childWidth = std::max(80.0f, measureTextWidth(child->content));
            } else if (child->type == NodeType::Image) {
                childWidth = parseStyleFloat(child, "width", 240.0f);
            } else {
                childWidth = std::max(80.0f, flexWidth);
            }
            layoutNode(child, cursorX, innerY, childWidth, innerHeight);
            cursorX += childWidth + gap;
        }
        return;
    }
}
