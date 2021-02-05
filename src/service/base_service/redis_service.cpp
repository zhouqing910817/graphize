#include "service/base_service/redis_service.h"
#include "common/context.h"
#include <google/protobuf/util/json_util.h>
#include "butil/third_party/snappy/snappy.h"
#include <functional>
#include <memory>
#include <atomic>
#include <list>
#include "frame/client/redis/redis_client_manager.h"

namespace graph_frame {
int REDIS_CLIENT = 0;
int ARESKV_CLIENT = 1;
void RedisService::init_fetcher_info() {
}
void RedisService::init_fetcher_info(std::shared_ptr<Context> context) {
}
void RedisService::init(std::shared_ptr<Context> context) {
    register_key_generator();
    register_response_processor();
    init_fetcher_info();
    init_fetcher_info(context);
}

void RedisService::register_key_generator() {
    register_key_func("imei", [](std::shared_ptr<Context> context, std::vector<std::string>& keys, const std::string& prefix, const std::string& postfix) -> int {
        return 0;
    });
}
std::shared_ptr<std::string> RedisService::make_redis_value(const std::string& key, std::shared_ptr<Context> context) {
    return std::shared_ptr<std::string>();
}
void RedisService::make_up_fetch_info_vec(std::shared_ptr<Context> context, std::shared_ptr<RedisResult> redis_result, std::vector<std::shared_ptr<const FetchInfo>>& fetch_info_vec) {
    std::vector<FetchInfo> add_fetch_info_vec;
    for (int i =0; i < fetch_info_vec.size(); i++) {
        auto & fetch_info = *(const_cast<FetchInfo*>(fetch_info_vec[i].get()));
        auto& prefix = fetch_info.prefix;
        auto& postfix = fetch_info.postfix;
        auto & keys = fetch_info.keys;
        auto key_func_it = key_func_map.find(fetch_info.type);
        if (key_func_it != key_func_map.end()) {
            key_func_it->second(context, keys, prefix, postfix);
        } else {
            std::cout << ", not exist type: " << fetch_info.type << " in key_func_map";
            continue;
        }
        if (keys.size() == 0) continue;
        // get from cache first if needed
        if (fetch_info.use_cache) {
            auto miss_keys_ptr = get_from_cache(keys, redis_result->cache_data_vec, fetch_info_vec[i]);
            std::cout << "[cache]" << "prefix:" << prefix << ", ratio:" << 1 - (miss_keys_ptr->size() / static_cast<float>(keys.size()));
            // <<  " miss_keys_size:" << miss_keys_ptr->size() << " keys_size:" << keys.size();
            // if (!miss_keys_ptr || miss_keys_ptr->size() <= 0) {
            //     continue;
            // }
            keys = std::move(*miss_keys_ptr);
        }
        if (keys.size() == 0) continue;
        if (fetch_info.client_type == REDIS_CLIENT) {
            auto new_redis_client = redis_client::RedisClientManager::instance().get_client(fetch_info.redis_client_id);
            if (!new_redis_client) {
                std::cout << ", cant find name : [" << fetch_info.redis_client_id << "] in RedisClientManager";
                continue;
            }
            // notice
            std::unordered_map<size_t, std::vector<std::string>> slot_keys_map;
            for (auto& key : keys) {
                size_t slot = new_redis_client->get_cur_channel_pool()->GetKeySlot(key.c_str(), key.size());
                slot_keys_map[slot].push_back(key);
            }
            std::cout << "slot_keys_map size: " << slot_keys_map.size() << " slot: " << slot_keys_map.begin()->first;
            auto begin_it = slot_keys_map.begin();
            for (auto it = slot_keys_map.begin(); it != slot_keys_map.end(); ++it) {
                // std::cout << "slot_index:" << it->first << " keys size: " << it->second.size();
                if (begin_it == it) {
                    // first element
                    fetch_info.keys = std::move(it->second);
                    continue;
                }
                // std::cout << "add fetch_info, " << "prefix:" << fetch_info.prefix << " redis_client_id: " << fetch_info.redis_client_id
                // << " key size: " << it->second.size();
                FetchInfo add_fetch_info;
                add_fetch_info.prefix = fetch_info.prefix;
                add_fetch_info.postfix = fetch_info.postfix;
                add_fetch_info.compress = fetch_info.compress;
                add_fetch_info.type = fetch_info.type;
                add_fetch_info.redis_client_id = fetch_info.redis_client_id;
                add_fetch_info.key = fetch_info.key;
                add_fetch_info.response_type = fetch_info.response_type;
                add_fetch_info.use_cache = fetch_info.use_cache;
                add_fetch_info.expire_time = fetch_info.expire_time;
                add_fetch_info.client_type = fetch_info.client_type;
                add_fetch_info.keys = std::move(it->second);
                add_fetch_info_vec.push_back(std::move(add_fetch_info));
            }
        }
    }
    for (auto& add_fetch_info : add_fetch_info_vec) {
        fetch_info_vec.push_back(std::make_shared<const FetchInfo>(std::move(add_fetch_info)));
    }
    for (int i = 0; i < fetch_info_vec.size(); i++) {
        auto & fetch_info = *(const_cast<FetchInfo*>(fetch_info_vec[i].get()));
        fetch_info.id = std::to_string(i);
    }
}
int RedisService::do_service(std::shared_ptr<Context> context) {
    auto start_time_ms = butil::gettimeofday_us();
    init(context);
    auto redis_result = std::make_shared<RedisResult>();
    redis_result->start_time_ms = start_time_ms;
    if (fetch_info_vec.size() <= 0) {
        std::cout << "prefixes is empty ";
        redis_result->code = RedisResult::CONF_ERROR;
        redis_result->error_msg = "fetch_info_vec is empty";
        redis_result->merge_cache_data();
        this->process_response(context, *redis_result);
        run_output_nodes_if_ready(context);
        return 0;
    }

    // fill fetch_info id, keys and new fetchinfo if it is redis 
    make_up_fetch_info_vec(context, redis_result, fetch_info_vec);
    // reserve result_map
    redis_result->result_map = std::make_shared<std::unordered_map<std::string, std::vector<RedisResultItem>>>();
    for (auto&& fetch_info_ptr : fetch_info_vec) {
        auto & prefix = fetch_info_ptr->prefix;
        auto& id = fetch_info_ptr->id.size() == 0 ? prefix : fetch_info_ptr->id;
        (*redis_result->result_map)[id] = std::vector<RedisResultItem>();
    }
    auto cond = std::make_shared<std::atomic<int>>(fetch_info_vec.size());
    auto cond_finish_func = [context, cond, this, redis_result](){
        int old = cond->fetch_sub(1);
        if (old == 1) {
            // VLOG_APP(ERROR) << "all redis key returned; _transport_name: " << _transport_name << " redis_type:" << redis_type;
            redis_result->end_rpc_time = butil::gettimeofday_us();
            if (redis_result->start_rpc_time != -1) {
                redis_result->total_rpc_time = redis_result->end_rpc_time - redis_result->start_rpc_time;
            }
            redis_result->merge_cache_data();
            this->process_response(context, *redis_result);
            run_output_nodes_if_ready(context);
        }
    };
    for (auto fetch_info_ptr : fetch_info_vec) {
        auto & fetch_info = *fetch_info_ptr;
        auto& prefix = fetch_info.prefix;
        auto& postfix = fetch_info.postfix;
        bool use_cache = fetch_info.use_cache;
        uint64_t expire_time = fetch_info.expire_time;
        auto& id = fetch_info_ptr->id.size() == 0 ? prefix : fetch_info_ptr->id;
        // std::cout << " process fetch_info id: " << id;
        auto& redis_result_item_vec =  (*redis_result->result_map)[id];
        std::shared_ptr<redis_client::RedisClient> new_redis_client;
        new_redis_client = redis_client::RedisClientManager::instance().get_client(fetch_info.redis_client_id);
        if (!new_redis_client) {
            std::cout << ", cant find redis client name : [" << fetch_info.redis_client_id << "] in RedisClientManager";
            cond_finish_func();
            continue;
        }
        // generate request keys
        const std::vector<std::string>& keys = fetch_info_ptr->keys;;
        if (keys.size() <= 0) {
            cond_finish_func();
            continue;
        }

        auto item_size_cond = std::make_shared<std::atomic<int>>(keys.size());
        int step = 1;
        // std::cout << "enable_multi_get:" << enable_multi_get;
        if (enable_multi_get) {
            // mget batch
            int batch_count = keys.size() / batch_size;
            if (keys.size() % batch_size != 0) {
                ++batch_count;
            }
            step = keys.size() / batch_count;
            if (keys.size() % batch_count != 0) {
                ++step;
            }
            item_size_cond = std::make_shared<std::atomic<int>>(batch_count);
            redis_result_item_vec.resize(batch_count);
            // std::cout << "step: " << step << " batch_count:" << batch_count;
        } else {
            redis_result_item_vec.resize(keys.size());
        }
        auto deep_cond_finish_func = [item_size_cond, redis_result, context, cond, this]() {
            int old_item_size = item_size_cond->fetch_sub(1);
            int old = -1;
            if (old_item_size == 1) {
                old = cond->fetch_sub(1);
            }
            if (old == 1 && old_item_size == 1) {
                redis_result->end_rpc_time = butil::gettimeofday_us();
                redis_result->total_rpc_time = redis_result->end_rpc_time - redis_result->start_rpc_time;
                redis_result->merge_cache_data();
                this->process_response(context, *redis_result);
                run_output_nodes_if_ready(context);
            }
        };
        for (int i = 0; i < keys.size(); i += step) {
            auto &key = keys.at(i);
            auto callback = [this, i, key, cond, item_size_cond, id, prefix, context, fetch_info_ptr, redis_result, deep_cond_finish_func, step]
                    (std::shared_ptr<::google::protobuf::Message> response_msg, std::shared_ptr<brpc::Controller> cntl_ptr) {
                try {
                    auto& keys = fetch_info_ptr->keys;
                    auto & redis_result_item_vec = (*redis_result->result_map)[id];
                    // std::cout << "fetch_info id: " << id;
                    auto & redis_result_item = redis_result_item_vec.at(i / step);
                    redis_result_item.fetch_info_ptr = fetch_info_ptr;
                    // std::cout << "redis_result: " << redis_result.get() << " set fetch_info_ptr:" << fetch_info_ptr.get() << " id:" << id << " index: " << i / step;
                    redis_result_item.m_get_size = std::min(static_cast<size_t>(i + step), keys.size()) - i;
                    redis_result_item.start_index = i;
                    redis_result_item.cntl_ptr = cntl_ptr;
                    redis_result_item.key = key;
                    std::shared_ptr<brpc::RedisResponse> response = std::dynamic_pointer_cast<brpc::RedisResponse>(response_msg);
                    auto &fetch_info = *fetch_info_ptr;
                    if (!cntl_ptr->Failed()) {
                        // 错误处理以及结果封装
                        check_suc_rpc_response(context, response, cntl_ptr, fetch_info_ptr, redis_result_item, key);
                    } else {
                        std::stringstream error_ss;
                        error_ss << " key:" << key << "redis_id: " << fetch_info_ptr->redis_client_id << " error_msg: " << cntl_ptr->ErrorText();
                        redis_result_item.error_msg = error_ss.str();
                        redis_result_item.code = RedisResultItem::RPC_FAILED;
                        std::cout << ", " << error_ss.str();
                    }
                    std::cout << "cntl_latency_us: " << cntl_ptr->latency_us() << " reply_size:" << (response == nullptr ? 0 : response->reply(0).size());
                    if (enable_paral_parse) {
                        int64_t lat = 0;
                        if (cntl_ptr != nullptr) {
                            lat = cntl_ptr->latency_us();
                        }
                        this->parse_redis_response(prefix, fetch_info.compress, response,
                            RedisRpcStatus(i, key, lat, redis_result_item.code),
                            context);
                    }
                    deep_cond_finish_func();
                } catch (const std::exception& e) {
                    std::cout << ", key: " << key << ", redis_client_id: " << fetch_info_ptr->redis_client_id 
                            << ", has exception:" <<  e.what();
                    deep_cond_finish_func();
                }
            };
            int ret = -1;
            // std::cout << "enable_multi_get: " << enable_multi_get;
            // std::cout << "fetch key: " << key;
            if (0 == i) {
                redis_result->start_rpc_time = butil::gettimeofday_us();
            }
            bool use_new_redis_client = false;
            if (!enable_multi_get) {
                // use new client first
                ret = new_redis_client->fetch(context, key, callback);
            } else {
                // std::cout << "start_index: " << i << " end_index:" << i + step;
                // use new client first
                ret = new_redis_client->multi_fetch(context, keys, i, i + step, callback);
            }
            if (ret == -1) {
                std::cout << "[client fetch fail] use_new_redis_client:" << use_new_redis_client;
                deep_cond_finish_func();
            }
        }
    }
    return 0;
}

void RedisService::check_suc_rpc_response(std::shared_ptr<graph_frame::Context> context, std::shared_ptr<brpc::RedisResponse>& response,
    std::shared_ptr<brpc::Controller> cntl_ptr, std::shared_ptr<const FetchInfo> fetch_info_ptr,
    RedisResultItem& redis_result_item, const std::string& key) {
    std::stringstream error_ss;
    if (response == nullptr) {
        error_ss << "response is nullptr or dynamic_pointer_cast failed, key:" << key
        << " redis_id: " << fetch_info_ptr->redis_client_id 
        << " error_msg: " << cntl_ptr->ErrorText();
        redis_result_item.error_msg = error_ss.str();
        redis_result_item.code = RedisResultItem::OTHER_ERROR;
        std::cout << ", " << redis_result_item.error_msg; 
    } else if (response->reply_size() <= 0) {
        response.reset();
        error_ss << " reply_size: " << response->reply_size()
        << " key:" << key << "redis_id: " << fetch_info_ptr->redis_client_id
        << " error_msg: " << cntl_ptr->ErrorText();
        redis_result_item.error_msg = error_ss.str();
        redis_result_item.code = RedisResultItem::OTHER_ERROR;
    } else if (response->reply(0).is_nil()) {
        response.reset();
        error_ss << " reply is nil;" << " key:" << key << " redis_id: " << fetch_info_ptr->redis_client_id
        << " error_msg: " << cntl_ptr->ErrorText();
        redis_result_item.error_msg = error_ss.str();
        redis_result_item.code = RedisResultItem::SUCCESS;
        redis_result_item.result_str = "";  // nil 结果是空串
        redis_result_item.result_str_vec.push_back(nullptr);
        redis_result_item.result_str_len_vec.push_back(0);
		std::cout << ", " << error_ss.str() << std::endl;
    } else if (!response->reply(0).is_string() && !response->reply(0).is_array()) {
        error_ss << " reply is not a string or array;" << " key:" << key << " redis_id: " << fetch_info_ptr->redis_client_id
        << " error_msg: " << cntl_ptr->ErrorText() << " error_message: " << response->reply(0).error_message();
        redis_result_item.error_msg = error_ss.str();
        redis_result_item.code = RedisResultItem::OTHER_ERROR;
        std::cout << ", " << error_ss.str();
        response.reset();
    } else {
        // std::cout << key << " redis data size: " << response->reply(0).data().size() << " redis_type:" << redis_type;
        redis_result_item.code = RedisResultItem::SUCCESS;
        if (response->reply(0).is_string()) {
            redis_result_item.result_str = response->reply(0).data().data();
            redis_result_item.result_str_vec.push_back(redis_result_item.result_str);
            redis_result_item.result_str_len_vec.push_back(response->reply(0).data().size());
            // std::cout << "result_str: " << redis_result_item.result_str;
        } else {
            // MGET 的结果存入result_str_vec中
            const auto& reply = response->reply(0);
            redis_result_item.result_str_vec.reserve(reply.size());
            redis_result_item.result_str_len_vec.reserve(reply.size());
            // std::cout << "array reply.size(): " << reply.size();
            for (int i = 0; i < reply.size(); i++) {
                const auto& sub_reply = reply[i];
                if (!sub_reply.is_nil()) {
                    redis_result_item.result_str_vec.push_back(sub_reply.data().data());
                    redis_result_item.result_str_len_vec.push_back(sub_reply.data().size());
                    // std::cout << "data: " << sub_reply.data().data();
                } else {
                    redis_result_item.result_str_vec.push_back(nullptr);
                    redis_result_item.result_str_len_vec.push_back(0);
                }
            }
        }
        redis_result_item.result_guard = response;
    }
}
void RedisService::parse_redis_response(const std::string& prefix, const std::string& compress,
    std::shared_ptr<brpc::RedisResponse> content_response, RedisRpcStatus status,
    std::shared_ptr<Context> context) {
        if (content_response) {
            if (compress == "snappy") {
                std::string uncompress_data_str = "";
                uncompress_data_str.reserve(content_response->reply(0).data().size() * 6); // snappy compress ratio is 22%, so mutilpy by 6
                auto ok = butil::snappy::Uncompress(content_response->reply(0).data().data(), content_response->reply(0).data().size(),
                    &uncompress_data_str);
                auto parse_func = get_parse_func(prefix);
                auto suc = parse_func(uncompress_data_str.c_str(), uncompress_data_str.size(), context, status);
                if (!suc) {
                    std::cout << "parse redis response to message failed; redis prefix: [" << prefix << status.key <<  "] uncompress ok: " << ok;
                }
            } else {
                auto parse_func = get_parse_func(prefix);
                auto suc = parse_func(content_response->reply(0).data().data(), content_response->reply(0).data().size(), context, status);
                if (!suc) {
                    std::cout << "parse redis response to message failed; redis prefix: [" << prefix << "]";
                } else {
                    std::cout << "parse redis suc; prefix:" << prefix;
                }
            }
        } else {
            // std::cout << "redis prefix: [" << prefix << "] get nullptr response, timeout or response is nil";
            auto parse_func = get_parse_func(prefix);
            auto suc = parse_func("", 0, context, status);
        }
}

std::shared_ptr<std::vector<std::string>> RedisService::get_from_cache(std::vector<std::string>& keys, std::vector<CacheDataItem>& cache_data_vec, std::shared_ptr<const FetchInfo> fetch_info_ptr) {
    std::shared_ptr<std::vector<std::string>> miss_keys = std::make_shared<std::vector<std::string>>();
	/*
    std::unordered_map<std::string, std::pair<const char*, size_t>> data_map;
    miss_keys->reserve(keys.size() * 0.3);
    auto& m_cache_mgr = Rec::Cache::CacheManager::instance();
    for (std::string& key : keys) {
        const char* value;
        size_t size;
        auto ret = m_cache_mgr.Get(key, value, size);
        if (ret == 0) {
            // std::cout << "Got key: " << key;
            if (size == 1 && value[0] == ' ') {
                value = "";
                size = 0;
            }
            data_map[key] = {value, size};
        } else {
            miss_keys->push_back(std::move(key));
        }
    }
    if (data_map.size() > 0) {
        CacheDataItem cache_data_item;
        cache_data_item.data_map = std::move(data_map);
        cache_data_item.fetch_info_ptr = fetch_info_ptr;
        cache_data_vec.push_back(std::move(cache_data_item));
    }
	*/
    return miss_keys;
}

void RedisService::process_response(std::shared_ptr<Context> context, const RedisResult& redis_result) {
}

void RedisResult::merge_cache_data(){
    // merge cache data into response
    // if (cache_data_map.size() > 0) {
    //     std::vector<RedisResultItem> item_vec;
    //     item_vec.resize(1);
    //     auto& item = item_vec[0];
    //     item.result_str_vec.reserve(cache_data_map.size());
    //     for (auto& pair : cache_data_map) {
    //          auto& key = pair.first;
    //          auto& value = pair.second;
    //          item.result_str_vec.push_back(value.data());
    //     }
    //     result_map->insert({"cache_data", std::move(item_vec)});
    // }

   // merge response to local cache
   /*
   auto& m_cache_mgr = Rec::Cache::CacheManager::instance();
   if (!result_map) return;
   for (auto & pair : *result_map) {
       auto & redis_item_vec = pair.second;
       if (redis_item_vec.size() == 0) continue;
       bool use_cache = redis_item_vec[0].fetch_info_ptr->use_cache;
       auto expire_time = redis_item_vec[0].fetch_info_ptr->expire_time;
       if (!use_cache) continue;
       for (auto& redis_item : redis_item_vec) {
           for (int i = 0; i < redis_item.result_str_vec.size(); i++) {
               const char* value = redis_item.result_str_vec[i];
               if (i >= redis_item.result_str_len_vec.size()) {
                   std::cout << "result_str_vec size[" << redis_item.result_str_vec.size() << "] not equal to result_str_len_vec[" << redis_item.result_str_len_vec.size() << "] size";
                   continue;
               }
               auto value_len = redis_item.result_str_len_vec[i];
               if (value == nullptr) {
                   // nil
                   value = " ";
                   value_len = 1;
               }
               const std::string key = redis_item.key;
               auto ret = m_cache_mgr.Set(key, value, value_len, expire_time);
               // std::cout << "set_cache: " << key << " ret: " << ret;
           }
       }
   }
   */
}
} // namespace graph_frame

