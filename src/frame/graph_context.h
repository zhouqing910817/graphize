#pragma once
#include <atomic>
#include <memory>
#include <unordered_map>
#include <string>
#include <vector>
#include "frame/register.h"
#include <typeinfo>
namespace brpc {
class Controller;
}
namespace google {
namespace protobuf {
class Closure;
}
}
namespace graph_frame {
class GenericServiceContext;
class Node;
extern std::vector<std::string> global_channel_name_vec;
extern std::unordered_map<std::string, std::vector<std::pair<std::string, std::string>>> global_graph_node_name_vec_map;
class GenericServiceContext {
	public:
	int64_t lat = -1;
};

template<typename Request, typename Response>
class GraphContext {
    
private:
    friend class Graph;
    friend class RedisService;
    friend class Node;
    std::vector<std::shared_ptr<std::atomic<int>>> node_input_num_map;
    std::vector<std::shared_ptr<GenericServiceContext>> node_service_context_vec;
    std::string graph_name;
    const Request* request;
    Response * response;

public:
void set_graph_name(const std::string& graph_name) {
    this->graph_name = graph_name;
}

std::shared_ptr<GenericServiceContext> get_context(int node_id) {
        if (node_id >= node_service_context_vec.size()) {
            LOG(FATAL) << "wrong node_id:" << node_id << ", node_service_context_vec.size():" << node_service_context_vec.size();
        }
	return node_service_context_vec[node_id];
}
GraphContext(const std::string& graph_name, const Request* req, Response* resp,brpc::Controller* cntl, google::protobuf::Closure* closure) : graph_name(graph_name), request(req), response(resp) {
}
};
} // end of namespace
