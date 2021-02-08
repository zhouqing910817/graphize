#include "frame/node_manager.h"
#include <iostream>
#include "frame/gtransport_manager.h"
#include "frame/client/redis/redis_client_manager.h"
#include "frame/global.h"
#include "butil/logging.h"
#include <algorithm>

namespace graph_frame {
bool NodeManager::init(const std::string& file_path) {
	hocon::shared_config root_conf = hocon::config::parse_file_any_syntax(file_path);
	hocon::shared_object root_obj = root_conf->root();
	const auto& node_name_vec = root_obj->key_set();
	for (const std::string& node_name : node_name_vec) {
		auto node_obj = root_conf->get_object(node_name);
		const auto& node_attr_key_vec = node_obj->key_set();
		auto node_conf = node_obj->to_config();
		std::string clazz = node_conf->get_string("class");
		std::string downstream = "";
		if (std::find(node_attr_key_vec.begin(), node_attr_key_vec.end(), "downstream") != node_attr_key_vec.end()) {
	        downstream = node_conf->get_string("downstream");
		}
        auto it = nodes_map.find(node_name);
        if (it != nodes_map.end()) {
            LOG(ERROR) << "node: " << node_name << " already exists";
        } else {
            nodes_map[node_name] = node_conf;
			LOG(ERROR) << "create node: " << node_name;
        }
        if (!downstream.empty()) {
            if (!GtransportManager::instance().get_transport(downstream) && !redis_client::RedisClientManager::instance().get_client(downstream)) {
                LOG(ERROR) << "conf error !! transport_name:" << downstream << " not exist in gtransport and redis client";
                std::cout << "conf error !! transport_name:" << downstream << " not exist in gtransport and redis client" << std::endl;
				return false;
            }
        }
	}
    return true;
}
} // end of namespace graph_frame
