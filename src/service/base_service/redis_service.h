#pragma once
#include <memory>
#include <functional>
#include <utility>
#include "common/context.h"
#include "frame/graph.h"
namespace graph_frame {
using graph_frame::Context;

extern int REDIS_CLIENT;
extern int ARESKV_CLIENT;
struct FetchInfo {
    std::string prefix;
    std::string postfix;
    std::string compress;
    std::string type;
    std::string redis_client_id;
    std::string key;
    std::string response_type;
    bool use_cache = false;
    uint64_t expire_time = 0;
    
    std::vector<std::string> keys;
    std::string id;
    int client_type = REDIS_CLIENT; // 用于区分redis的key 按照slot拆分的逻辑
};
class RedisResultItem {
    public:
    RedisResultItem() : result_str(nullptr), code(OTHER_ERROR) {}
    RedisResultItem(const char* result_str, std::shared_ptr<brpc::RedisResponse> result_guard, int code, const std::string& error_msg):
    result_str(result_str), result_guard(result_guard), code(code), error_msg(error_msg) {}

    std::shared_ptr<const FetchInfo> fetch_info_ptr; // to performance
    std::string key; // get 时使用
    const char* result_str; // get 时使用
    std::vector<const char*> result_str_vec;
    std::vector<size_t> result_str_len_vec;
    uint64_t total_result_str_size = 0; // 用于监控日志
    int start_index = 0; // mget 起始的key的下标
    int m_get_size = 0; // 监控本次mget命令参数的个数
    std::shared_ptr<brpc::Controller> cntl_ptr;
    int code;
    std::string error_msg;
    const char* data() {return result_str;}
    int error_code() {return code;};
    std::string err_msg() {return error_msg;};

    public:
    std::shared_ptr<brpc::RedisResponse> result_guard;
public:
    static const int SUCCESS = 0; // rpc 访问成功且数据正确(数据为空也是正确数据)
    static const int RPC_FAILED = 1; // rpc 访问失败
    static const int OTHER_ERROR = 2; // 配置文件错误或者rpc成功但数据格式不对
};
class CacheDataItem {
public:
    std::unordered_map<std::string, std::pair<const char*, size_t>> data_map;
    std::shared_ptr<const FetchInfo> fetch_info_ptr;
};
class RedisResult {
    public:
    std::shared_ptr<std::unordered_map<std::string, std::vector<RedisResultItem>>> result_map;
    static const int CONF_ERROR = 0;
    int code;
    std::string error_msg;
    uint64_t start_time_ms = -1;
    uint64_t start_rpc_time = -1;
    uint64_t end_rpc_time = -1;

    uint64_t total_rpc_time = -1;
    // 缓存数据
    std::vector<CacheDataItem> cache_data_vec;
    void merge_cache_data();
};
struct RedisRpcStatus {
    RedisRpcStatus(int lat, int status):lat(lat), status(status){}
    RedisRpcStatus(int index, std::string key, int lat, int status):
        index(index),key(std::move(key)), lat(lat), status(status) {
    }
    int index;
    std::string key;
    int lat = 0;
    int status = 3; // 0:suc, 1:timeout, 2:not exists, 4:other error

    std::shared_ptr<std::vector<const char*>> result_str_vec;
    std::shared_ptr<std::vector<std::shared_ptr<brpc::RedisResponse>>> redis_response_vec; // guard result_str_vec
};
class RedisService : public Node{
  public:
    RedisService(const std::string& class_name) : Node(class_name) {
    }
    virtual void init(std::shared_ptr<Context> context);
    int do_service(std::shared_ptr<Context> context) override;
    // 子类需要实现的函数
    virtual void process_response(std::shared_ptr<Context> context, const RedisResult& redis_result);
    virtual std::shared_ptr<std::string> make_redis_value(const std::string& key, std::shared_ptr<Context> context);
    virtual void init_fetcher_info();
    virtual void init_fetcher_info(std::shared_ptr<Context> context);
    virtual void register_key_generator();
    virtual void register_response_processor(){};

    void parse_redis_response(const std::string& prefix, const std::string& compress,
        std::shared_ptr<brpc::RedisResponse> content_response, RedisRpcStatus status,
        std::shared_ptr<Context> context);
    const std::string type() override {
        return "io";
    }
    void register_key_func(const std::string& type, std::function<int(std::shared_ptr<Context>, std::vector<std::string>&, const std::string&, const std::string&)> func) {
        key_func_map.insert({type, func});
    }
    void register_parse_func(const std::string& prefix, std::function<int(const char*, size_t, std::shared_ptr<Context>, RedisRpcStatus status)> func) {
        parse_func_map.insert({prefix, func});
    }
    std::function<int(const char*, size_t, std::shared_ptr<Context>, RedisRpcStatus)> & get_parse_func(const std::string& prefix) {
        auto it = parse_func_map.find(prefix);
        if (it != parse_func_map.end()) {
            return it->second;
        }
        static std::function<int(const char*, size_t, std::shared_ptr<Context>, RedisRpcStatus)> default_func = [](const char*, size_t, std::shared_ptr<Context>, RedisRpcStatus){return 1;};
        return default_func;
    }
    void check_suc_rpc_response(std::shared_ptr<graph_frame::Context> context, std::shared_ptr<brpc::RedisResponse>& response,
        std::shared_ptr<brpc::Controller> cntl_ptr, std::shared_ptr<const FetchInfo> fetch_info_ptr,
        RedisResultItem& redis_result_item, const std::string& key);
    std::shared_ptr<std::vector<std::string>> get_from_cache(std::vector<std::string>& keys, std::vector<CacheDataItem>& cache_data_vec, std::shared_ptr<const FetchInfo> fetch_info_ptr);
    void make_up_fetch_info_vec(std::shared_ptr<Context> context, std::shared_ptr<RedisResult> redis_result, std::vector<std::shared_ptr<const FetchInfo>>& fetch_info_vec);
  protected:
    std::unordered_map<std::string, std::function<int(std::shared_ptr<Context>, const char*, size_t)>> response_func_map;
    std::vector<std::shared_ptr<const FetchInfo>> fetch_info_vec;
    std::string _transport_name;
    std::unordered_map<std::string, std::function<int(const char*, size_t, std::shared_ptr<Context>, RedisRpcStatus)>> parse_func_map;
    std::unordered_map<std::string, std::function<int(std::shared_ptr<Context>,  std::vector<std::string>&, const std::string&, const std::string&)>> key_func_map;
    std::string _fetch_conf_path;
public:
    bool enable_paral_parse = false;

    bool enable_multi_get = true;
    int batch_size = 100;
};
} // end of namespace graph_frame

