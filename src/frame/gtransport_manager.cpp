#include "frame/gtransport_manager.h"
#include <iostream>
#include "frame/global.h"
#include "butil/logging.h"
#include <hocon/config.hpp>

namespace graph_frame{
GtransportManager& GtransportManager::instance() {
    static GtransportManager inst;
    return inst;
}

void GtransportManager::init(const std::string& conf_path) noexcept {
    // LOG(ERROR) << "downstream_map size: " << downstream_map.size();
    // std::cout << "downstream_map size: " << downstream_map.size() << std::endl;
	hocon::shared_config root_conf = hocon::config::parse_file_any_syntax(conf_path);
	hocon::shared_object root_obj = root_conf->root();
	const auto& downstream_name_vec = root_obj->key_set();
	for (const std::string& downstream_name : downstream_name_vec) {
		auto ds_object = root_conf->get_object(downstream_name);
		auto ds_conf = ds_object->to_config();
		std::string ns = ds_conf->get_string("ns");
        std::string lb = ds_conf->get_string("load_balancer");
        int timeout = ds_conf->get_int("timeout_ms");
        int backup_timeout_ms = ds_conf->get_int("backup_timeout_ms");
        std::string connection_type = ds_conf->get_string("connection_type");
        int max_retry = ds_conf->get_int("max_retry");
        std::string protocol = ds_conf->get_string("protocal");
        std::shared_ptr<brpc::Channel> channel(new brpc::Channel());

        brpc::ChannelOptions options;
        options.protocol = protocol.c_str();
        options.connection_type = connection_type.c_str();
        options.timeout_ms = timeout;
        options.max_retry = max_retry;
        options.backup_request_ms = backup_timeout_ms;

		if (channel->Init(ns.c_str(), lb.c_str(), &options) != 0) {
             LOG(WARNING) << "Fail to initialize " << downstream_name << " channel";
             return;
         }
         if (downstream_map.find(downstream_name) == downstream_map.end()) {
             LOG(WARNING) << "insert downstream: " << downstream_name << " ns: " << ns;
             std::cout << "insert downstream" << downstream_name << " ns: " << ns << std::endl;
             downstream_map[downstream_name] = channel;
         } else {
             LOG(ERROR) << "downstream" << downstream_name << " already exists! error!!";
         }
	}
}

std::shared_ptr<brpc::Channel> GtransportManager::get_transport(const std::string& transport_name) {
    auto it = downstream_map.find(transport_name);
    
    if (it != downstream_map.end()) {
        return it->second;
    } else {
        LOG(ERROR) << "transport_name: "<< transport_name.c_str() << " not found";
        return std::shared_ptr<brpc::Channel>();
    }
}
} // end of namespace graph_frame
