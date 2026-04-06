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
    
    // Find the longest line to determine the overall width
    size_t start = 0;
    size_t end = text.find('\n');
    size_t longestLineSize = 0;
    
    while (end != std::string::npos) {
        longestLineSize = std::max(longestLineSize, end - start);
        start = end + 1;
        end = text.find('\n', start);
    }
    longestLineSize = std::max(longestLineSize, text.length() - start);
    
    return (static_cast<float>(longestLineSize) * charWidth) + 16.0f;
}

static float measureTextHeight(const std::string& text, float maxWidth, float fontSize) {
    const float charWidth = std::max(1.0f, fontSize * 0.55f);
    const float lineHeight = std::max(1.0f, fontSize * 1.25f);
    if (text.empty()) {
        return lineHeight + 16.0f; // Padding
    }
    
    float availableWidth = std::max(1.0f, maxWidth - 16.0f); // Accounting for 8px left/right padding
    int columns = std::max(1, static_cast<int>(std::floor(availableWidth / charWidth)));
    
    int totalWrappedLines = 0;
    size_t start = 0;
    size_t end = text.find('\n');
    
    while (true) {
        size_t lineLength = (end == std::string::npos) ? text.length() - start : end - start;
        // Even an empty line (two \n in a row) counts as one line
        int wrappedForThisLine = (lineLength == 0) ? 1 : std::max(1, static_cast<int>((lineLength + columns - 1) / columns));
        totalWrappedLines += wrappedForThisLine;
        
        if (end == std::string::npos) break;
        start = end + 1;
        end = text.find('\n', start);
    }
    
    return (lineHeight * static_cast<float>(totalWrappedLines)) + 16.0f; // Accounting for 8px top/bottom padding
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
        float totalHeight = 0.0f;
        if (!node->children.empty()) {
            layoutNode(node->children[0], x, y, width, height);
            totalHeight = node->children[0]->height;
        }
        node->height = std::max(height, totalHeight); // Ensure it's at least as tall as viewport
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
    
    // If the node has its own text, we space the children below/beside it
    float headerHeight = 0.0f;
    float headerWidth = 0.0f;
    if (!content.empty()) {
        float fontSize = parseStyleFloat(node, "fontsize", 16.0f);
        headerHeight = measureTextHeight(content, width > 0.0f ? width : 2000.0f, fontSize);
        headerWidth = measureTextWidth(content, fontSize);
    }

    float innerX = x + padding + (node->type == NodeType::FlexH ? headerWidth : 0.0f);
    float innerY = y + padding + (node->type == NodeType::FlexV ? headerHeight : 0.0f);
    float innerWidth = std::max(0.0f, width - padding * 2.0f - (node->type == NodeType::FlexH ? headerWidth : 0.0f));
    float innerHeight = std::max(0.0f, height - padding * 2.0f - (node->type == NodeType::FlexV ? headerHeight : 0.0f));
    
    const int count = static_cast<int>(node->children.size());
    if (count == 0) {
        // If we have text but no children, node size is already set above
        return;
    }
    
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
            cursorY += child->height;
        }

        // --- DYNAMIC HEIGHT FIX ---
        // Update the container's final height to fit its content if it's taller than the initial height
        float contentHeight = (cursorY - innerY) + padding * 2.0f;
        node->height = std::max(height, contentHeight);
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
                childWidth = std::max(40.0f, flexWidth);
            }

            layoutNode(child, cursorX, innerY, childWidth, innerHeight);
            cursorX += child->width;
        }

        // --- DYNAMIC HEIGHT FIX FOR FLEXH ---
        float maxChildHeight = 0.0f;
        for (Node* child : node->children) {
            maxChildHeight = std::max(maxChildHeight, child->height);
        }
        node->height = std::max(height, maxChildHeight + padding * 2.0f);
        node->width = std::max(width, (cursorX - innerX) + padding * 2.0f);
        return;
    }
}
