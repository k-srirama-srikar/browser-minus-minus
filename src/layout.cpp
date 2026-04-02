#include "layout.hpp"
#include <algorithm>
#include <cmath>
#include <string>
#include <iostream>

static float parseStyleFloat(const Node* node, const std::string& key, float fallback) {
    if (node->properties.contains(key) && node->properties[key].is_number()) {
        return node->properties[key].get<float>();
    }
    if (node->properties.contains("text") && node->properties["text"].is_object()) {
        if (node->properties["text"].contains(key) && node->properties["text"][key].is_number()) {
            return node->properties["text"][key].get<float>();
        }
    }
    return fallback;
}

static std::string getNodeText(const Node* node) {
    if (node->properties.contains("text") && node->properties["text"].is_object()) {
        if (node->properties["text"].contains("content") && node->properties["text"]["content"].is_string()) {
            return node->properties["text"]["content"].get<std::string>();
        }
    }
    return "";
}

static float measureTextWidth(const std::string& text, float fontSize) {
    const float charWidth = std::max(1.0f, fontSize * 0.55f);
    return std::max(0.0f, static_cast<float>(text.size()) * charWidth);
}

static float measureTextHeight(const std::string& text, float maxWidth, float fontSize) {
    const float charWidth = std::max(1.0f, fontSize * 0.55f);
    const float lineHeight = std::max(1.0f, fontSize * 1.25f);
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
    
    // JSML root might just be a shell, let's just lay it out with 0 padding
    if (node->type == NodeType::Root) {
        if (!node->children.empty()) {
            layoutNode(node->children[0], x, y, width, height);
        }
        return;
    }

    if (node->type == NodeType::Text) {
        std::string content = getNodeText(node);
        float fontSize = parseStyleFloat(node, "fontsize", 16.0f);
        float effectiveWidth = width > 0.0f ? width : measureTextWidth(content, fontSize);
        node->width = effectiveWidth;
        node->height = measureTextHeight(content, effectiveWidth, fontSize);
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
    float gap = parseStyleFloat(node, "spacing", 8.0f);
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
                float fontSize = parseStyleFloat(child, "fontsize", 16.0f);
                fixedHeight += measureTextHeight(getNodeText(child), innerWidth, fontSize);
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
                float fontSize = parseStyleFloat(child, "fontsize", 16.0f);
                childHeight = measureTextHeight(getNodeText(child), innerWidth, fontSize);
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
                float fontSize = parseStyleFloat(child, "fontsize", 16.0f);
                fixedWidth += measureTextWidth(getNodeText(child), fontSize);
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
                float fontSize = parseStyleFloat(child, "fontsize", 16.0f);
                float measured = measureTextWidth(getNodeText(child), fontSize);
                childWidth = std::max(80.0f, measured);
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
