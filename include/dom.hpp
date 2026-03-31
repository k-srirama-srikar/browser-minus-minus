#include <string>
#include <vector>
#include <variant>
#include <map> 

enum class NodeType {FlexV, FlexH, Text, Image};

struct Node{
    int id; // unique id for lua interop
    NodeType type;
    std::map<std::string, std::string> attrs;
    std::string content;
    std::vector<Node*> children;
    float x, y, width, height; // geometric bounds to be calculated by the layout engine
};