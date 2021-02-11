#ifndef _OBJECT_SHM_H
#define _OBJECT_SHM_H

#include <string>
#include "logger.h"
#include "shm_hashtable.h"
#include "shmcache.h"
#include "butil/logging.h"

extern "C"{
    int log_init();
}
namespace objectcache
{
class ObjectCache
{
public:
    ObjectCache(){};
    int InitCache(std::string ini_cfg){
        log_init();
        init_status_ = shmcache_init_from_file(&ctx_, ini_cfg.c_str());
        // if (init_status_ != 0) {
        //     LOG(ERROR) << "shmcache_init failed, retry ...";
        //     shmcache_remove_all(&ctx_);
        //     init_status_ = shmcache_init_from_file(&ctx_, ini_cfg.c_str());
        //     LOG(ERROR) << "shmcache_init failed, retry ret: " << init_status_;
        // }
        return init_status_;
    };
    
    int set(const std::string& key, const char* value, size_t value_len, long ttl) {
        if(init_status_ != 0){
            return init_status_;
        }

		shmcache_key_info key_info;

        key_info.length = key.length();
        key_info.data = const_cast<char*>(key.data());

		shmcache_value_info value_info;
        value_info.data = const_cast<char*>(value);
        value_info.length = value_len;
        value_info.options = SHMCACHE_SERIALIZER_STRING;
        time_t current_time = time(NULL);
        value_info.expires = current_time + ttl;

        int ret = shmcache_set_ex(&ctx_, &key_info, &value_info);
        return ret;
    }

    int get(const std::string& key, const char*& value, size_t& value_size) {
        if(init_status_ != 0){
            return init_status_;
        }

		shmcache_key_info key_info;
        key_info.length = key.length();
        key_info.data = const_cast<char*>(key.data());
        shmcache_value_info value_info;
        int ret = shmcache_get(&ctx_, &key_info, &value_info);
        if(ret != 0){
            return ret;
        }

        value = value_info.data;
        value_size = value_info.length;
        return 0;
    }
public:
    struct shmcache_context ctx_;
    int init_status_;
};

}

#endif
