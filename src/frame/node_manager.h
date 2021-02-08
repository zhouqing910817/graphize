#pragma once
#include <unordered_map>
#include <memory>
#include <iostream>
#include <hocon/config.hpp>

namespace graph_frame{
class NodeManager {
private:
    NodeManager() {
    };
    std::unordered_map<std::string, hocon::shared_config> nodes_map;
public:
    static NodeManager& instance() {
        static NodeManager inst;
        return inst;
    }

    bool init(const std::string& file_path);
    hocon::shared_config get_node_info(const std::string& node_name) {
        auto it = nodes_map.find(node_name);
        if (it != nodes_map.end()) {
            return it->second;
        } else {
            std::cout << "node_name: " << node_name << " found in nodes_map" << std::endl;
            return hocon::shared_config();
        }
    }
};
} // end of namespace graph_frame
