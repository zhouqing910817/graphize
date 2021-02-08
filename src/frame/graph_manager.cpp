#include "frame/graph_manager.h"
#include "frame/node_manager.h"
#include "frame/graph.h"
#include <boost/algorithm/string.hpp>
#include <hocon/config.hpp>
namespace graph_frame{
bool GraphManager::init(const std::string& file_path) {
    Node* root = nullptr;
    std::unordered_map<std::string, Node*> nodes_map;
    auto create_service_if_not_exist = [&nodes_map](const std::string name, std::vector<std::pair<std::string, std::string>>& graph_node_vec) ->Node* {
        auto it = nodes_map.find(name);
        if (it != nodes_map.end()) {
            return it->second;
        }
        auto node_config = NodeManager::instance().get_node_info(name);
        if (!node_config) {
            LOG(ERROR) << "node name: " << name << " not found in node manager";
            return nullptr;
        }

        std::string clazz = node_config->get_string("class");
        auto service_fac = GET_SERVICE_FACTORY(clazz);
        if (!service_fac) {
            LOG(ERROR) << "unregistered class: [" << clazz << "]";
            return nullptr;
        }
        auto service_node = service_fac->create();
        service_node->init(node_config);
        service_node->set_name(name);
        nodes_map[name] = service_node;
		graph_node_vec.push_back({name, clazz});
        return service_node;
    };
	hocon::shared_config root_conf = hocon::config::parse_file_any_syntax(file_path);
	hocon::shared_object root_obj = root_conf->root();
	const auto& graph_name_vec = root_obj->key_set();
	for (const std::string & graph_name : graph_name_vec) {
		std::shared_ptr<Graph> graph = std::make_shared<Graph>();
		auto graph_obj = root_conf->get_object(graph_name);
		auto graph_conf = graph_obj->to_config();
		const auto& node_name_vec = graph_obj->key_set();
		Node* root = nullptr;
		std::vector<std::pair<std::string, std::string>>& graph_node_name_vec = global_graph_node_name_vec_map[graph_name];
		for (const auto & node_name : node_name_vec) {
			auto graph_node_obj = graph_conf->get_object(node_name);
			auto graph_node_conf = graph_node_obj->to_config();
            Node* service = create_service_if_not_exist(node_name, graph_node_name_vec);
            if (!service) {
                LOG(ERROR) << "graph node name: " << node_name << " create failed";
		    	break;
            }
			const auto& graph_node_attrs = graph_node_obj->key_set();
			bool is_root = false;
			if (std::find(graph_node_attrs.begin(), graph_node_attrs.end(), "root") != graph_node_attrs.end()) {
			    is_root = graph_node_conf->get_bool("root");
			}
			if (is_root) {
				root = service;
			}
			// parse children
            {
			std::string children = "";
			if (std::find(graph_node_attrs.begin(), graph_node_attrs.end(), "children") != graph_node_attrs.end()) {
			    children = graph_node_conf->get_string("children");
			}
            boost::trim(children);
            if (!children.empty()) {
                std::vector<std::string> child_vec;
                boost::split(child_vec, children, boost::is_any_of(","));
                for (std::string& c : child_vec) {
					if(c.empty()) continue;
                    boost::trim(c);
                    Node* child = create_service_if_not_exist(c, graph_node_name_vec);
                    if (!child) {
                        continue;
                    }
					const auto & output_nodes = service->get_output_nodes();
					if (std::find(output_nodes.begin(), output_nodes.end(),child) == output_nodes.end()) {
                        service->add_output(child);
					}
                }
            }
            }

			// parse parent
            {
			std::string parent = "";
			if (std::find(graph_node_attrs.begin(), graph_node_attrs.end(), "parents") != graph_node_attrs.end()) {
			    parent = graph_node_conf->get_string("parents");
			}
            boost::trim(parent);
            if (!parent.empty()) {
                 std::vector<std::string> p_vec;
                 boost::split(p_vec, parent, boost::is_any_of(","));
                 for (std::string& p : p_vec) {
                     boost::trim(p);
                     Node* p_node = create_service_if_not_exist(p, graph_node_name_vec);
                     if(!p_node) {
                         continue;
                     }
					 const auto& output_nodes = p_node->get_output_nodes();
					 if (std::find(output_nodes.begin(), output_nodes.end(), service) == output_nodes.end()) {
                         p_node->add_output(service);
				     }
                 }
            }
            }
		}
		LOG(ERROR) << "graph:" << graph_name << " graph_node_name_vec size: " << graph_node_name_vec.size();
        auto ret = graph->init(root, graph_name);
		if (ret == 0) {
            graph_map.insert({graph_name, graph});
			return true;
		} else {
			LOG(FATAL) << "init " << graph_name << " failed, it's not a DAG";
			return false;
		}
	}
}

std::shared_ptr<Graph> GraphManager::get_graph(const std::string& topo_name) {
    auto it = graph_map.find(topo_name);
    if (it == graph_map.end()) {
		LOG(FATAL) << "graph: " << topo_name << " not found";
        return std::shared_ptr<Graph>();
    } else {
        return it->second;
    }
}
} // end of namespace
