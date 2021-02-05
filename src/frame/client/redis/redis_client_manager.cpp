#include "frame/client/redis/redis_client_manager.h"
#include <hocon/config.hpp>

namespace redis_client {
bool RedisClientManager::init(const std::string& path) {
	hocon::shared_config root_conf = hocon::config::parse_file_any_syntax(path);
	hocon::shared_object root_obj = root_conf->root();
	const auto& name_vec = root_obj->key_set();
	for (const std::string& name : name_vec) {
        auto redis_obj  = root_conf->get_object("name");
		auto redis_conf = redis_obj->to_config();
		std::string address = redis_conf->get_string("address");
		int pool_size = redis_conf->get_int("pool_size");
		int max_pool_size = redis_conf->get_int("max_pool_size");
		int timeout = redis_conf->get_int("timeout");
        std::cout << "init redis name: " << name << " address: " << address << std::endl;
        LOG(ERROR) << "init redis name: " << name << " address: " << address;
        std::shared_ptr<RedisClient> client_ptr = std::make_shared<RedisClient>(address, "", timeout, pool_size, max_pool_size, 0);
        client_map.insert({name, client_ptr});
	}
	
    LOG(ERROR) << "client_map size: " << client_map.size();
    std::cout << "client_map size: " << client_map.size() << std::endl;

    return true;
}

std::shared_ptr<RedisClient> RedisClientManager::get_client(const std::string& id) {
    auto it = client_map.find(id);
    if(it != client_map.end()) {
        return it->second;
    }
    // todo print error message
    LOG(ERROR) << "redis client not find: " << id;
    return std::shared_ptr<RedisClient>();
}

} // end of namespace client
