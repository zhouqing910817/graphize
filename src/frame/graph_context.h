#pragma once
#include <atomic>
#include <memory>
#include <unordered_map>
namespace graph_frame {
class Node;
class GraphContext {
    public:
    std::unordered_map<Node*, std::shared_ptr<std::atomic<int>>> node_input_num_map;
	GraphContext() {
	}
};
} // end of namespace
