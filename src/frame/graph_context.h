#pragma once
#include <atomic>
#include <memory>
#include <unordered_map>
#include <string>
#include <vector>
#include "frame/register.h"
#include <typeinfo>
namespace graph_frame {
class GenericServiceContext;
class Node;
extern std::vector<std::string> global_channel_name_vec;
extern std::unordered_map<std::string, std::vector<std::pair<std::string, std::string>>> global_graph_node_name_vec_map;
class GenericServiceContext {
	public:
	int64_t lat = -1;
};
class GraphContext {
    public:
    std::unordered_map<Node*, std::shared_ptr<std::atomic<int>>> node_input_num_map;
    std::unordered_map<std::string, std::shared_ptr<GenericServiceContext>> node_service_context_map;

	std::shared_ptr<GenericServiceContext> get_context(const std::string& node_name) {
		auto it = node_service_context_map.find(node_name);
		if (it != node_service_context_map.end()) {
			// LOG(ERROR) << "found service context for node: " << node_name << " context: " << it->second.get();
			return it->second;
		} else {
			LOG(FATAL) << "can't found service context for node:" << node_name << " this should not be happen";
		}
		return node_service_context_map[node_name];
	}
	GraphContext(const std::string& graph_name) : graph_name(graph_name) {
		auto it = global_graph_node_name_vec_map.find(graph_name);
		
		if (it == global_graph_node_name_vec_map.end()) {
			 LOG(FATAL) << "cant't found graph_name: " << graph_name << " in global_graph_node_name_vec_map, this should not happen";
		}
		const auto & node_name_vec = it->second;
		// init service context
        for (const auto& node_name_clazz_pair : node_name_vec) {
		    node_service_context_map[node_name_clazz_pair.first].reset(GET_SERVICE_FACTORY(node_name_clazz_pair.second)->create_context());
			// LOG(ERROR) << "graph_name:" << graph_name << " node_name:" << node_name_clazz_pair.first <<
			//	   	" class_name: " << node_name_clazz_pair.second << " create context" << " type: " << typeid(*node_service_context_map[node_name_clazz_pair.first]).name();
		}
	}
	private:
	std::string graph_name;
};
} // end of namespace
