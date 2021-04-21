#include "service/base/redis_service.h"
#include "common/context.h"
#include <google/protobuf/util/json_util.h>
#include "butil/third_party/snappy/snappy.h"
#include <functional>
#include <memory>
#include <atomic>
#include <list>
#include "frame/client/redis/redis_client_manager.h"
#include "frame/cache_manager.h"

namespace graph_frame {

// void RedisService::register_key_generator() {
//     register_key_func("", [](std::shared_ptr<Context> context, std::vector<std::string>& keys, const std::string& prefix, const std::string& postfix) -> int {
//         return 0;
//     });
// }
DEFINE_bool(redis_service_debug, false, "debug if the graph run is configured");
void RedisService::init(hocon::shared_config conf) {
    auto fetch_info_obj_list = conf->get_object_list("fetch_infos");
    for (auto fetch_info_obj : fetch_info_obj_list) {
        auto fetch_info_conf = fetch_info_obj->to_config();
        const auto& key_vec = fetch_info_obj->key_set();
        FetchInfo fetch_info;
        fetch_info.prefix = fetch_info_conf->get_string("prefix");
        fetch_info.postfix = fetch_info_conf->get_string("postfix");
        fetch_info.redis_client_id = fetch_info_conf->get_string("redis_client_id");
        fetch_info.key_gen_func = fetch_info_conf->get_string("key_gen_func");
        fetch_info.response_func = fetch_info_conf->get_string("response_func");
        fetch_info.compress = fetch_info_conf->get_string("compress");

        fetch_info.use_cache = false;
        if (std::find(key_vec.begin(), key_vec.end(), "use_cache") != key_vec.end()) {
            fetch_info.use_cache = fetch_info_conf->get_bool("use_cache");
        }
        fetch_info.expire_time = 0;
        if (std::find(key_vec.begin(), key_vec.end(), "expire_time_s") != key_vec.end()) {
            fetch_info.expire_time = fetch_info_conf->get_int("expire_time_s");
        }
        fetch_info_vec.push_back(std::make_shared<const FetchInfo>(std::move(fetch_info)));
    }
    init_redis_conf(conf);
}
std::shared_ptr<std::string> RedisService::make_redis_value(const std::string& key, std::shared_ptr<Context> context) {
    return std::shared_ptr<std::string>();
}
void RedisService::make_up_fetch_info_vec(std::shared_ptr<Context> context, std::shared_ptr<RedisResult> redis_result, std::vector<std::shared_ptr<const FetchInfo>>& fetch_info_vec) {
    std::vector<FetchInfo> add_fetch_info_vec;
    int old_fetch_info_size = fetch_info_vec.size();
    for (int i =0; i < fetch_info_vec.size(); i++) {
        auto & fetch_info = *(const_cast<FetchInfo*>(fetch_info_vec[i].get()));
        auto& prefix = fetch_info.prefix;
        auto& postfix = fetch_info.postfix;
        auto & keys = fetch_info.keys;
        auto key_func_it = key_func_map.find(fetch_info.key_gen_func);
        if (key_func_it != key_func_map.end()) {
            key_func_it->second(context, keys, prefix, postfix);
        } else {
            LOG(ERROR) << "not exist key_gen_func: " << fetch_info.key_gen_func << " in key_func_map";
            continue;
        }
        if (keys.size() == 0) continue;
        // get from cache first if needed
        if (fetch_info.use_cache) {
            auto miss_keys_ptr = get_from_cache(keys, redis_result->cache_data_vec, fetch_info_vec[i]);
            LOG(ERROR) << "[cache]" << "prefix:" << prefix << ", ratio:" << 1 - (miss_keys_ptr->size() / static_cast<float>(keys.size()))
                <<  " miss_keys_size:" << miss_keys_ptr->size() << " keys_size:" << keys.size();
            if (!miss_keys_ptr) {
                continue;
            }
            keys = std::move(*miss_keys_ptr);
        }
        if (keys.size() == 0) continue;
        auto new_redis_client = redis_client::RedisClientManager::instance().get_client(fetch_info.redis_client_id);
        if (!new_redis_client) {
            LOG(ERROR) << ", cant find name : [" << fetch_info.redis_client_id << "] in RedisClientManager";
            continue;
        }
        std::unordered_map<size_t, std::vector<std::string>> slot_keys_map;
        for (auto& key : keys) {
            size_t slot = new_redis_client->get_cur_channel_pool()->GetKeySlot(key.c_str(), key.size());
            slot_keys_map[slot].push_back(key);
        }
        // LOG(ERROR) << "slot_keys_map size: " << slot_keys_map.size() << " slot: " << slot_keys_map.begin()->first;
        auto begin_it = slot_keys_map.begin();
        for (auto it = slot_keys_map.begin(); it != slot_keys_map.end(); ++it) {
            // LOG(ERROR) << "slot_index:" << it->first << " keys size: " << it->second.size();
            if (begin_it == it) {
                // first element
                fetch_info.keys = std::move(it->second);
                continue;
            }
            // LOG(ERROR) << "add fetch_info, " << "prefix:" << fetch_info.prefix << " redis_client_id: " << fetch_info.redis_client_id
            // << " key size: " << it->second.size();
            FetchInfo add_fetch_info;
            add_fetch_info.prefix = fetch_info.prefix;
            add_fetch_info.postfix = fetch_info.postfix;
            add_fetch_info.compress = fetch_info.compress;
            add_fetch_info.key_gen_func = fetch_info.key_gen_func;
            add_fetch_info.redis_client_id = fetch_info.redis_client_id;
            add_fetch_info.key = fetch_info.key;
            add_fetch_info.response_func = fetch_info.response_func;
            add_fetch_info.use_cache = fetch_info.use_cache;
            add_fetch_info.expire_time = fetch_info.expire_time;
            add_fetch_info.keys = std::move(it->second);
            add_fetch_info_vec.push_back(std::move(add_fetch_info));
        }
    }
    for (auto& add_fetch_info : add_fetch_info_vec) {
        fetch_info_vec.push_back(std::make_shared<const FetchInfo>(std::move(add_fetch_info)));
    }
    for (int i = 0; i < fetch_info_vec.size(); i++) {
        auto & fetch_info = *(const_cast<FetchInfo*>(fetch_info_vec[i].get()));
        fetch_info.id = std::to_string(i);
    }
    if (old_fetch_info_size != fetch_info_vec.size() && FLAGS_redis_service_debug) {
        LOG(ERROR) << "old_fetch_info_size: " << old_fetch_info_size << " cur_fetch_info_size:" << fetch_info_vec.size();
    }
}
int RedisService::do_service(std::shared_ptr<Context> context) {
    auto start_time_ms = butil::gettimeofday_us();
    auto redis_result = std::make_shared<RedisResult>();
    redis_result->start_time_ms = start_time_ms;
    if (fetch_info_vec.size() <= 0) {
        if (FLAGS_redis_service_debug) {
            LOG(ERROR) << "prefixes is empty ";
        }
        redis_result->code = RedisResult::CONF_ERROR;
        redis_result->error_msg = "fetch_info_vec is empty";
        redis_result->merge_cache_data();
        this->process_response(context, *redis_result);
        run_output_nodes_if_ready(context);
        return 0;
    }

    // fill fetch_info id, keys and new fetchinfo if it is redis 
    make_up_fetch_info_vec(context, redis_result, fetch_info_vec);
    std::shared_ptr<std::promise<bool>> cache_data_promise_ptr = redis_result->cache_data_vec.size() > 0 ? std::make_shared<std::promise<bool>>() : std::shared_ptr<std::promise<bool>>();
    // reserve result_map
    redis_result->result_map = std::make_shared<std::unordered_map<std::string, std::vector<RedisResultItem>>>();
    for (auto&& fetch_info_ptr : fetch_info_vec) {
        auto & prefix = fetch_info_ptr->prefix;
        auto& id = fetch_info_ptr->id.size() == 0 ? prefix : fetch_info_ptr->id;
        (*redis_result->result_map)[id] = std::vector<RedisResultItem>();
    }
    auto cond = std::make_shared<std::atomic<int>>(fetch_info_vec.size());
    auto cond_finish_func = [context, cond, this, redis_result, cache_data_promise_ptr](bool send_rpc){
        int old = cond->fetch_sub(1);
        if (old == 1) {
            if (FLAGS_redis_service_debug) {
                LOG(ERROR) << "all redis key returned";
            }
            redis_result->end_rpc_time = butil::gettimeofday_us();
            if (redis_result->start_rpc_time != -1) {
                redis_result->total_rpc_time = redis_result->end_rpc_time - redis_result->start_rpc_time;
            }
            redis_result->merge_cache_data();
            if(cache_data_promise_ptr) {
                // wait cache data finish
                if (!send_rpc) {
                    // LOG(ERROR) << "not send rpc, parse cached data";
                    parse_cached_data(redis_result->cache_data_vec, context, cache_data_promise_ptr);
                }
                if (FLAGS_redis_service_debug) {
                    LOG(ERROR) << "waiting cache_data_promise";
                }
                cache_data_promise_ptr->get_future().get();
                if (FLAGS_redis_service_debug) {
                    LOG(ERROR) << "waiting cache_data_promise back";
                }
            }
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
        // LOG(ERROR) << " process fetch_info id: " << id;
        auto& redis_result_item_vec =  (*redis_result->result_map)[id];
        std::shared_ptr<redis_client::RedisClient> new_redis_client;
        new_redis_client = redis_client::RedisClientManager::instance().get_client(fetch_info.redis_client_id);
        if (!new_redis_client) {
            LOG(ERROR) << "can't find redis client name : [" << fetch_info.redis_client_id << "] in RedisClientManager";
            cond_finish_func(false);
            continue;
        }
        // generate request keys
        const std::vector<std::string>& keys = fetch_info_ptr->keys;
        if (FLAGS_redis_service_debug) {
            LOG(ERROR) << "to fetch keys size: " << keys.size();
        }
        if (keys.size() <= 0) {
            cond_finish_func(false);
            continue;
        }

        auto item_size_cond = std::make_shared<std::atomic<int>>(keys.size());
        int step = 1;
        if (FLAGS_redis_service_debug) {
            LOG(ERROR) << "enable_multi_get:" << enable_multi_get;
        }
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
            // LOG(ERROR) << "step: " << step << " batch_count:" << batch_count;
        } else {
            redis_result_item_vec.resize(keys.size());
        }
        auto deep_cond_finish_func = [item_size_cond, redis_result, context, cond, this, cache_data_promise_ptr](bool send_rpc) {
            int old_item_size = item_size_cond->fetch_sub(1);
            int old = -1;
            if (old_item_size == 1) {
                old = cond->fetch_sub(1);
            }
            if (old == 1 && old_item_size == 1) {
                if (FLAGS_redis_service_debug) {
                    LOG(ERROR) << "all back";
                }
                redis_result->end_rpc_time = butil::gettimeofday_us();
                redis_result->total_rpc_time = redis_result->end_rpc_time - redis_result->start_rpc_time;
                redis_result->merge_cache_data();
               if(cache_data_promise_ptr) {
                    // wait cache data finish
                   if (!send_rpc) {
                       parse_cached_data(redis_result->cache_data_vec, context, cache_data_promise_ptr);
                   }
                   if (FLAGS_redis_service_debug) {
                       LOG(ERROR) << "waiting cache_data_promise";
                   }
                   cache_data_promise_ptr->get_future().get();
                   if (FLAGS_redis_service_debug) {
                       LOG(ERROR) << "waiting cache_data_promise back";
                   }
               }
                this->process_response(context, *redis_result);
                run_output_nodes_if_ready(context);
            } else {
                if (FLAGS_redis_service_debug) {
                    LOG(ERROR) << "not all back";
                }
            }
        };
        for (int i = 0; i < keys.size(); i += step) {
            auto &key = keys.at(i);
            auto callback = [this, i, key, cond, item_size_cond, id, prefix, context, fetch_info_ptr, redis_result, deep_cond_finish_func, step]
                    (std::shared_ptr<::google::protobuf::Message> response_msg, std::shared_ptr<brpc::Controller> cntl_ptr) {
                try {
                    if (FLAGS_redis_service_debug) {
                        LOG(ERROR) << "callback back start_index:" << i;
                    }
                    auto& keys = fetch_info_ptr->keys;
                    auto & redis_result_item_vec = (*redis_result->result_map)[id];
                    // LOG(ERROR) << "fetch_info id: " << id;
                    auto & redis_result_item = redis_result_item_vec.at(i / step);
                    redis_result_item.fetch_info_ptr = fetch_info_ptr;
                    // LOG(ERROR) << "redis_result: " << redis_result.get() << " set fetch_info_ptr:" << fetch_info_ptr.get() << " id:" << id << " index: " << i / step;
                    redis_result_item.m_get_size = std::min(static_cast<size_t>(i + step), keys.size()) - i;
                    redis_result_item.start_index = i;
                    redis_result_item.cntl_ptr = cntl_ptr;
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
                        LOG(ERROR) << ", " << error_ss.str();
                    }
                    if (enable_paral_parse) {
                        int64_t lat = 0;
                        if (cntl_ptr != nullptr) {
                            lat = cntl_ptr->latency_us();
                        }
                        this->parse_redis_response(prefix, i / step, redis_result_item_vec.size(), redis_result_item, context);
                    }
                    LOG(ERROR) << "[MONITOR] " << " prefix=" << fetch_info_ptr->prefix << " suc=" << (!cntl_ptr->Failed())
                               << " r_lat=" << cntl_ptr->latency_us() << " m_k=" << redis_result_item.m_get_size
                               << " d_size=" << redis_result_item.total_result_str_size;
                    deep_cond_finish_func(true);
                } catch (const std::exception& e) {
                    LOG(ERROR) << ", key: " << key << ", redis_client_id: " << fetch_info_ptr->redis_client_id 
                            << ", has exception:" <<  e.what();
                    deep_cond_finish_func(true);
                }
            };
            int ret = -1;
            // LOG(ERROR) << "enable_multi_get: " << enable_multi_get;
            // LOG(ERROR) << "fetch key: " << key;
            if (0 == i) {
                redis_result->start_rpc_time = butil::gettimeofday_us();
            }
            bool use_new_redis_client = false;
            if (!enable_multi_get) {
                ret = new_redis_client->fetch(context, key, callback);
            } else {
                if (FLAGS_redis_service_debug) {
                    LOG(ERROR) << "start_index: " << i << " end_index:" << std::min(static_cast<size_t>(i + step), keys.size());
                }
                // use new client first
                ret = new_redis_client->multi_fetch(context, keys, i, i + step, callback);
            }
            if (ret == -1) {
                LOG(ERROR) << "[client fetch fail] use_new_redis_client:" << use_new_redis_client;
                deep_cond_finish_func(false);
            }
        }
    }

    //处理缓存数据
    if (FLAGS_redis_service_debug) {
        LOG(ERROR) << "cache_data_vec.size: " << redis_result->cache_data_vec.size();
    }
    if (redis_result->cache_data_vec.size() > 0) {
        parse_cached_data(redis_result->cache_data_vec, context, cache_data_promise_ptr);
    }
    return 0;
}

void RedisService::parse_cached_data(std::vector<CacheDataItem>& cache_data_vec, std::shared_ptr<Context> context, std::shared_ptr<std::promise<bool>> cache_data_promise_ptr) {

    for (const CacheDataItem& cache_data_item : cache_data_vec) {
        const auto & fetch_info = cache_data_item.fetch_info_ptr;
        if (!fetch_info->use_cache) {
            continue;
        }
        auto parse_func = get_parse_func(fetch_info->response_func);
        if (fetch_info->compress == "snappy") {
            for (const auto& pair : cache_data_item.data_map) {
                const std::string& key = pair.first;
                const auto& data_pair = pair.second;
                const char* result_str = data_pair.first;
                auto result_str_len = data_pair.second;
                std::string uncompress_data_str = "";
                uncompress_data_str.reserve(result_str_len * 6); // snappy compress ratio is 22%, so mutilpy by 6
                auto ok = butil::snappy::Uncompress(result_str, result_str_len, &uncompress_data_str);
                auto service_context = context->get_context(id);
                const std::string& prefix = fetch_info->prefix;
                auto suc = parse_func(key, uncompress_data_str.c_str(), uncompress_data_str.size(), 0, 1, service_context, fetch_info, true);
                if (!suc) {
                    LOG(ERROR) << "parse cached redis response to message failed; redis prefix: [" << prefix << "] key:[" << key <<"] uncompress ok: " << ok;
                }
            }
        } else {
            for (const auto& pair : cache_data_item.data_map) {
                const std::string& key = pair.first;
                const auto& data_pair = pair.second;
                const char* result_str = data_pair.first;
                auto result_str_len = data_pair.second;
                auto service_context = context->get_context(id);
                const std::string& prefix = fetch_info->prefix;
                auto suc = parse_func(key, result_str, result_str_len, 0, 1, service_context, fetch_info, true);
                if (!suc) {
                    LOG(ERROR) << "parse cached redis response to message failed; redis prefix: [" << prefix << "] key:[" << key << "]";
                } else {
                    // LOG(ERROR) << "parse cached redis suc; prefix:" << prefix;
                }
            }
        }
    }
    // 缓存数据用完后清理掉，防止解析两次
    // （如果最后的fetchinfo没有发出远程调用，不清理掉缓存数据会被解析两次,此函数会被串行执行两次）
    cache_data_vec.clear();
    try {
        //  可能会被set_value两次
        cache_data_promise_ptr->set_value(true);
    } catch (const std::future_error& error) {
    }
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
        LOG(ERROR) << ", " << redis_result_item.error_msg; 
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
        redis_result_item.result_str_vec.push_back(nullptr);
        redis_result_item.result_str_len_vec.push_back(0);
        LOG(ERROR) << ", " << error_ss.str() << std::endl;
    } else if (!response->reply(0).is_string() && !response->reply(0).is_array()) {
        error_ss << " reply is not a string or array;" << " key:" << key << " redis_id: " << fetch_info_ptr->redis_client_id
        << " error_msg: " << cntl_ptr->ErrorText() << " error_message: " << response->reply(0).error_message();
        redis_result_item.error_msg = error_ss.str();
        redis_result_item.code = RedisResultItem::OTHER_ERROR;
        LOG(ERROR) << ", " << error_ss.str();
        response.reset();
    } else {
        // LOG(ERROR) << key << " redis data size: " << response->reply(0).data().size() << " redis_type:" << redis_type;
        redis_result_item.code = RedisResultItem::SUCCESS;
        if (response->reply(0).is_string()) {
            redis_result_item.result_str_vec.push_back(response->reply(0).data().data());
            redis_result_item.result_str_len_vec.push_back(response->reply(0).data().size());
            redis_result_item.total_result_str_size += response->reply(0).data().size();
        } else {
            // MGET 的结果存入result_str_vec中
            const auto& reply = response->reply(0);
            redis_result_item.result_str_vec.reserve(reply.size());
            redis_result_item.result_str_len_vec.reserve(reply.size());
            // LOG(ERROR) << "array reply.size(): " << reply.size();
            for (int i = 0; i < reply.size(); i++) {
                const auto& sub_reply = reply[i];
                if (!sub_reply.is_nil()) {
                    redis_result_item.result_str_vec.push_back(sub_reply.data().data());
                    redis_result_item.result_str_len_vec.push_back(sub_reply.data().size());
                    redis_result_item.total_result_str_size += sub_reply.data().size();
                    // LOG(ERROR) << "data: " << sub_reply.data().data();
                } else {
                    redis_result_item.result_str_vec.push_back(nullptr);
                    redis_result_item.result_str_len_vec.push_back(0);
                }
            }
        }
        redis_result_item.result_guard = response;
    }
}
void RedisService::parse_redis_response(const std::string& prefix, int redis_item_index, int redis_item_size, const RedisResultItem& redis_item, std::shared_ptr<Context> context) {
    auto parse_func = get_parse_func(redis_item.fetch_info_ptr->response_func);
    auto service_context = context->get_context(id);
    if (redis_item.fetch_info_ptr->compress == "snappy") {
        for (size_t i = 0; i < redis_item.result_str_vec.size(); ++i) {
            const char* result_str = redis_item.result_str_vec[i];
            auto result_str_len = redis_item.result_str_len_vec[i];
            std::string uncompress_data_str = "";
            uncompress_data_str.reserve(result_str_len * 6); // snappy compress ratio is 22%, so mutilpy by 6
            auto ok = butil::snappy::Uncompress(result_str, result_str_len, &uncompress_data_str);
            const std::string& key = redis_item.fetch_info_ptr->keys.at(redis_item.start_index + i);
            auto suc = parse_func(key, uncompress_data_str.c_str(), uncompress_data_str.size(), redis_item_index, redis_item_size, service_context, redis_item.fetch_info_ptr, false);
            if (!suc) {
                LOG(ERROR) << "parse redis response to message failed; redis prefix: [" << prefix << "] key:[" << key <<"] uncompress ok: " << ok;
            }
        }
    } else {
        for (size_t i = 0; i < redis_item.result_str_vec.size(); ++i) {
            const char* result_str = redis_item.result_str_vec[i];
            auto result_str_len = redis_item.result_str_len_vec[i];
            const std::string& key = redis_item.fetch_info_ptr->keys.at(redis_item.start_index + i);
            // LOG(ERROR) << "redis_item.start_index: " << redis_item.start_index << " i:" << i << " keys size:" << redis_item.fetch_info_ptr->keys.size()
            // << " result_str_vec.size: " << redis_item.result_str_vec.size()
            // << " service_context:" << service_context.get();
            auto suc = parse_func(key, result_str, result_str_len, redis_item_index, redis_item_size, service_context, redis_item.fetch_info_ptr, false);
            if (!suc) {
                LOG(ERROR) << "parse redis response to message failed; redis prefix: [" << prefix << "] key:[" << key << "]";
            } else {
                // LOG(ERROR) << "parse redis suc; prefix:" << prefix;
            }
        }
    }
}

std::shared_ptr<std::vector<std::string>> RedisService::get_from_cache(std::vector<std::string>& keys, std::vector<CacheDataItem>& cache_data_vec, std::shared_ptr<const FetchInfo> fetch_info_ptr) {
    std::shared_ptr<std::vector<std::string>> miss_keys = std::make_shared<std::vector<std::string>>();
    std::unordered_map<std::string, std::pair<const char*, size_t>> data_map;
    miss_keys->reserve(keys.size() * 0.3);
    auto& m_cache_mgr = CacheManager::instance();
    for (std::string& key : keys) {
        const char* value;
        size_t size;
        auto ret = m_cache_mgr.Get(key, value, size);
        if (ret == 0) {
            // LOG(ERROR) << "Got key: " << key;
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
   auto& m_cache_mgr = CacheManager::instance();
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
                   LOG(ERROR) << "result_str_vec size[" << redis_item.result_str_vec.size() << "] not equal to result_str_len_vec[" << redis_item.result_str_len_vec.size() << "] size";
                   continue;
               }
               auto value_len = redis_item.result_str_len_vec[i];
               if (value == nullptr) {
                   // nil
                   value = " ";
                   value_len = 1;
               }
               const std::string& key = redis_item.fetch_info_ptr->keys.at(redis_item.start_index + i);
               auto ret = m_cache_mgr.Set(key, value, value_len, expire_time);
               LOG(ERROR) << "set_cache: " << key << " ret: " << ret;
           }
       }
   }
}
} // namespace graph_frame

