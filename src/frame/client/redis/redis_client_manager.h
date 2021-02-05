#pragma once

#include "frame/client/redis/redis_client.h"
#include <unordered_map>
namespace redis_client {

class RedisClientManager {
private:
    RedisClientManager(){}
    std::unordered_map<std::string, std::shared_ptr<RedisClient>> client_map;
public:
    bool init(const std::string& path);
    std::shared_ptr<RedisClient> get_client(const std::string& id);
    static RedisClientManager& instance() {
        static RedisClientManager inst;
        return inst;
    }
};

} // end of namespace client
