#pragma once
namespace graph_frame { 
class CacheManager{
public:
    CacheManager() {};
    static CacheManager& instance() {
        static CacheManager _cache_manager;
        return _cache_manager;
    }
    int init(const std::string& file_path);
    int Get(std::string& key, const char*& value, size_t& size);
    int Set(const std::string& key, const char* value, size_t value_len, int expired_time);
};
}

