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
    if (text.empty()) return 0.0f;
    const float charWidth = std::max(1.0f, fontSize * 0.55f);
    // Add 16px for left+right padding (8px each)
    return (static_cast<float>(text.size()) * charWidth) + 16.0f;
}

static float measureTextHeight(const std::string& text, float maxWidth, float fontSize) {
    const float charWidth = std::max(1.0f, fontSize * 0.55f);
    const float lineHeight = std::max(1.0f, fontSize * 1.25f);
    if (text.empty()) {
        return lineHeight + 16.0f; // Padding
    }
    
    float availableWidth = std::max(1.0f, maxWidth - 16.0f); // Accounting for 8px left/right padding
    int columns = std::max(1, static_cast<int>(std::floor(availableWidth / charWidth)));
    int lines = static_cast<int>((text.size() + columns - 1) / columns);
    return (lineHeight * static_cast<float>(lines)) + 16.0f; // Accounting for 8px top/bottom padding
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

    // Nodes with Text trait (could be any node type now)
    std::string content = getNodeText(node);
    if (!content.empty()) {
        float fontSize = parseStyleFloat(node, "fontsize", 16.0f);
        float measuredWidth = measureTextWidth(content, fontSize);
        float measuredHeight = measureTextHeight(content, width > 0.0f ? width : measuredWidth, fontSize);
        
        // If width/height were not provided by parent, use measured values
        if (node->width <= 0.0f) node->width = measuredWidth;
        if (node->height <= 0.0f) node->height = measuredHeight;
        
        // If it's JUST a text node, we can return. Otherwise, it might have children to layout below/within.
        if (node->type == NodeType::Text) return;
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
        float totalSpacing = 0.0f;

        for (int i = 0; i < count; ++i) {
            Node* child = node->children[i];
            float childSpacing = parseStyleFloat(child, "spacing", 8.0f);
            if (i > 0) totalSpacing += childSpacing;

            if (!getNodeText(child).empty()) {
                float fontSize = parseStyleFloat(child, "fontsize", 16.0f);
                fixedHeight += measureTextHeight(getNodeText(child), innerWidth, fontSize);
            } else if (child->type == NodeType::Image) {
                fixedHeight += parseStyleFloat(child, "height", 140.0f);
            } else {
                flexCount++;
            }
        }

        float remaining = innerHeight - fixedHeight - totalSpacing;
        float flexHeight = flexCount > 0 ? std::max(40.0f, remaining / static_cast<float>(flexCount)) : 0.0f;

        float cursorY = innerY;
        for (int i = 0; i < count; ++i) {
            Node* child = node->children[i];
            float childSpacing = parseStyleFloat(child, "spacing", 8.0f);
            if (i > 0) cursorY += childSpacing;

            float childHeight = 0.0f;
            if (!getNodeText(child).empty()) {
                float fontSize = parseStyleFloat(child, "fontsize", 16.0f);
                childHeight = measureTextHeight(getNodeText(child), innerWidth, fontSize);
            } else if (child->type == NodeType::Image) {
                childHeight = parseStyleFloat(child, "height", 140.0f);
            } else {
                childHeight = std::max(40.0f, flexHeight);
            }
            layoutNode(child, innerX, cursorY, innerWidth, childHeight);
            cursorY += childHeight;
        }
        return;
    }

    if (node->type == NodeType::FlexH) {
        float fixedWidth = 0.0f;
        int flexCount = 0;
        float totalSpacing = 0.0f;

        for (int i = 0; i < count; ++i) {
            Node* child = node->children[i];
            float childSpacing = parseStyleFloat(child, "spacing", 8.0f);
            if (i > 0) totalSpacing += childSpacing;

            if (!getNodeText(child).empty()) {
                float fontSize = parseStyleFloat(child, "fontsize", 16.0f);
                fixedWidth += measureTextWidth(getNodeText(child), fontSize);
            } else if (child->type == NodeType::Image) {
                fixedWidth += parseStyleFloat(child, "width", 240.0f);
            } else {
                flexCount++;
            }
        }

        float remaining = innerWidth - fixedWidth - totalSpacing;
        float flexWidth = flexCount > 0 ? std::max(80.0f, remaining / static_cast<float>(flexCount)) : 0.0f;

        float cursorX = innerX;
        for (int i = 0; i < count; ++i) {
            Node* child = node->children[i];
            float childSpacing = parseStyleFloat(child, "spacing", 8.0f);
            if (i > 0) cursorX += childSpacing;

            float childWidth = 0.0f;
            if (!getNodeText(child).empty()) {
                float fontSize = parseStyleFloat(child, "fontsize", 16.0f);
                childWidth = measureTextWidth(getNodeText(child), fontSize);
            } else if (child->type == NodeType::Image) {
                childWidth = parseStyleFloat(child, "width", 240.0f);
            } else {
                childWidth = std::max(80.0f, flexWidth);
            }
            layoutNode(child, cursorX, innerY, childWidth, innerHeight);
            cursorX += childWidth;
        }
        return;
    }
}
