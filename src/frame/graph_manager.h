#pragma once
#include <string>
#include <unordered_map>
#include <memory>
namespace graph_frame {
class Graph;
class Node;

bool init_graph_frame(const std::string& graph_frame_file);

class GraphManager {
private:
    GraphManager(){}
public:
    static GraphManager& instance() {
        static GraphManager inst;
        return inst;
    }

    bool init(const std::string& conf_path);
    std::shared_ptr<Graph> get_graph(const std::string& topo_name);
private:
    Node* create_service(const std::string name);
    std::unordered_map<std::string, std::shared_ptr<Graph>> graph_map;
};
} // end of namespace graph_frame
