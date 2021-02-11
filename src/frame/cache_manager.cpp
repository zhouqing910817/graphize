#include "single_object.h"
#include "objectcache.h"
#include "frame/cache_manager.h"
#include "butil/logging.h"

namespace graph_frame { 

int CacheManager::init(const std::string& file_path) {
    
    objectcache::ObjectCache *object_cache = SingletonObject<objectcache::ObjectCache>::GetInstance();
    if (object_cache == nullptr) {
        LOG(ERROR) << "object_cache nullptr";
        return -1;
    }

    int ret = object_cache->InitCache(file_path);
    if (ret != 0) {
        LOG(ERROR) << "cache init error:" << ret;
        return ret;
    }

    LOG(INFO) << "cache init success: " << file_path;
    return 0;
}

int CacheManager::Get(std::string& key, const char*& value, size_t& size){
    if (key.empty()) {
        return -1;
    }

    objectcache::ObjectCache *object_cache = SingletonObject<objectcache::ObjectCache>::GetInstance();
    int ret = -1;
    
    ret = object_cache->get(key, value, size);
    
    if (ret == 0) {
        return 0;
    }

    return -1;
}

int CacheManager::Set(const std::string& key, const char* value, size_t value_len, int expired_time){
    if (key.empty() || value == nullptr || value_len == 0 || expired_time <= 0) {
        return -1;
    }

    objectcache::ObjectCache *object_cache = SingletonObject<objectcache::ObjectCache>::GetInstance();

    return object_cache->set(key, value, value_len, expired_time);
}

}
