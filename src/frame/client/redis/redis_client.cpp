#include "frame/client/redis/redis_client.h"
#include <vector>
#include <memory>
#include "brpc/callback.h"
#include "bthread/bthread.h"
#include "bthread/id.h"
#include <boost/algorithm/string.hpp>
#include <chrono>
#include <boost/algorithm/string.hpp>
#include "butil/logging.h"

namespace redis_client {

//   rr                           # round robin, choose next server
//   random                       # randomly choose a server
//   la                           # locality aware
//   c_murmurhash/c_md5           # consistent hashing with murmurhash3/md5
//   "" or NULL  
RedisClient::RedisClient(const std::string& url, const std::string& load_balancer, uint64_t time_out, int pool_size, int max_pool_size, int max_retry) noexcept :
    _url(url), _lb(load_balancer), _time_out(time_out), _max_retry(max_retry), _pool_size(pool_size) {
    channel_pool_vec.resize(2);
    channel_pool_vec[0] = std::make_shared<ChannelPool>();
    channel_pool_vec[1] = std::make_shared<ChannelPool>();
    pool_index.store(0, std::memory_order_release);
    int pool_index_t = pool_index.load(std::memory_order_acquire);
    auto channel_pool = channel_pool_vec[pool_index];
    init_channel_pool(channel_pool);
    // thread_ptr = std::make_shared<std::thread>(&RedisClient::update_channel_pool, this);
    // thread_ptr->detach();
}

void RedisClient::update_channel_pool() {
    while(1) {
        std::this_thread::sleep_for(std::chrono::seconds(60));
        try {
            auto bak_pool_index = 1 - pool_index.load(std::memory_order_acquire);
            auto channel_pool = channel_pool_vec[bak_pool_index];
            bool rpc_suc = true;
            try {
                init_channel_pool(channel_pool);
            } catch (const std::exception & e) {
                LOG(ERROR) << "exception: " << e.what();
                rpc_suc = false;
            }
            if (!rpc_suc) return;
            if (is_channel_pool_changed()) {
                pool_index.store(bak_pool_index, std::memory_order_release);
                auto cur_index = pool_index.load(std::memory_order_acquire);
                LOG(ERROR) << "switch channel_pool cur_idex: " << cur_index;
                // channel_pool_vec[cur_index] = std::make_shared<ChannelPool>();
            } else {
                LOG(ERROR) << "channel pool not changed";
            }
        } catch (const std::exception & e) {
            LOG(ERROR) << "exception : " << e.what();
        }
    }
}

bool RedisClient::is_channel_pool_changed() {
    auto channel_pool0 = channel_pool_vec[0];
    auto channel_pool1 = channel_pool_vec[1];
    if (channel_pool0->valid_lines.size() != channel_pool1->valid_lines.size()) {
        LOG(ERROR) << "valid_lines size changed;  valid_lines 0 size: " << channel_pool0->valid_lines.size();
        LOG(ERROR) << "valid_lines size changed;  valid_lines 1 size: " << channel_pool1->valid_lines.size();
        return true;
    }
    for (int i = 0; i < channel_pool0->valid_lines.size(); i++) {
        std::vector<std::string> fields_0;
        std::vector<std::string> fields_1;
        boost::split(fields_0, channel_pool0->valid_lines[i], boost::is_any_of(" "), boost::token_compress_on);
        boost::split(fields_1, channel_pool1->valid_lines[i], boost::is_any_of(" "), boost::token_compress_on);
        auto log_func = [i, &channel_pool0, &channel_pool1](){
            LOG(ERROR) << "valied line 0: " << channel_pool0->valid_lines[i];
            LOG(ERROR) << "valied line 1: " << channel_pool1->valid_lines[i];
        };
        if (fields_0.size() < 8 || fields_1.size() < 8) {
            LOG(ERROR) << "size less than 8, " << fields_0.size() << " " << fields_1.size();
            LOG(ERROR) << "=========== fields_0 ============";
            for (auto & s : fields_0) {
                LOG(ERROR) << s.c_str();
            }
            LOG(ERROR) << "=========== end fields_0 ============";
            LOG(ERROR) << "=========== fields_1 ============";
            for (auto & s : fields_1) {
                LOG(ERROR) << s.c_str();
            }
            LOG(ERROR) << "=========== end fields_1 ============";
            log_func();
            return true;
        }
        if (fields_0.size() != fields_1.size()) {
            LOG(ERROR) << "size not equal" <<" fields_0 size: " << fields_0.size() << " fields_1 size: " << fields_1.size();
            log_func();
            return true;
        }
        if (fields_0[0] != fields_1[0] || fields_0[1] != fields_1[1]) {
            LOG(ERROR) << "0 or 1 not the same";
            log_func();
            return true;
        }
        for (int j = 8; j < fields_1.size(); ++j) {
            if (fields_0[j] != fields_1[j]) {
                LOG(ERROR) << "j: " << j << " not the same";
                log_func();
                return true;
            }
        }
    }
    return false;
}

void RedisClient::init_channel_pool(std::shared_ptr<ChannelPool> channel_pool) {
    brpc::ChannelOptions options;
    options.max_retry = _max_retry; // 0 means no retry
    options.timeout_ms = _time_out * 10;
    options.protocol = brpc::PROTOCOL_REDIS;
    brpc::Channel redis_channel;
    if (redis_channel.Init(_url.c_str(), &options) != 0) {  // 6379 is the default port for redis-server
           LOG(ERROR) << "Fail to init channel to redis-server: " << _url.c_str()
                      << " load balancer: " << _lb.c_str();
       throw "Fail to init channel to redis-server";
    }
    LOG(ERROR) << "url: " << _url.c_str() << " start";
    brpc::RedisRequest set_request;
    brpc::RedisResponse response;
    brpc::Controller cntl;
    set_request.AddCommand("cluster nodes");
    redis_channel.CallMethod(NULL, &cntl, &set_request, &response, NULL/*done*/);
    if (cntl.Failed()) {
        std::cout << "cntl failed, remote_ip:" << cntl.remote_side()
                     << " cost_us:" << cntl.latency_us() 
                     << " error:"<< cntl.ErrorText();
        LOG(ERROR) << "Fail to access redis-server[cluster nodes]";
        return;
    }
    // LOG(ERROR) << "reply_size: " << response.reply_size();
    if (response.reply(0).is_error()) {
        LOG(ERROR) << "Fail to set response";
        return;
    } 
    std::vector<std::string> vlines;
    // do clear
    channel_pool->valid_lines.clear();
    std::string s = std::string(response.reply(0).data().data(), response.reply(0).data().size());
    boost::trim(s);
    LOG(ERROR) << "response size: " << response.reply(0).data().size();
	LOG(ERROR) << "cluster nodes str: \n" << s;
    boost::split(vlines, s, boost::is_any_of("\n"), boost::token_compress_on);
	LOG(ERROR) << "vlines size: " << vlines.size();
    std::vector<NodeInfo> vNodes;
    for (size_t i= 0; i < vlines.size(); ++i) {
        NodeInfo node;
        node.strinfo = vlines[i];

        std::vector<std::string> nodeinfo;
        boost::split(nodeinfo, node.strinfo, boost::is_any_of(" "), boost::token_compress_on);
        if (nodeinfo.size() < 8) {
            continue;
        }
        if (NULL != strstr(nodeinfo[7].c_str(), "disconnected")){
            continue;
        }
        if (NULL == strstr(nodeinfo[2].c_str(), "master")) {
            continue;
        }
        channel_pool->valid_lines.push_back(vlines[i]);
        node.id = nodeinfo[0];
        node.ParseNodeString(nodeinfo[1]);
        // LOG(ERROR) << "ip: " << node.ip << " port:" << node.port;
        for (int j = 8; j < nodeinfo.size(); j++) {
            // LOG(ERROR) << "nodeinfo 8: " << nodeinfo[j];
            node.ParseSlotString(nodeinfo[j]);
        }
        vNodes.push_back(node);
    }
    if (!is_channel_pool_changed()) {
        LOG(ERROR) << " is_channel_pool_changed not changed, return without build redis client pool";
        return;
    }
    LOG(ERROR) << "url: " << _url.c_str() << " end";
    InitSlotNodeMap(channel_pool->slot_ipport_index_vec, vNodes);

    int32_t cnt = vNodes.size();

    if(channel_pool->m_channel_list.size() != cnt)channel_pool->m_channel_list.resize(cnt);
    LOG(ERROR) << "vnodes size: " << cnt;
    for (int32_t i = 0; i < cnt; ++i) {
		if (vNodes[i].ip.empty() && vNodes[i].port > 0) {
			vNodes[i].ip = "127.0.0.1";
		}
        InitChannel(i, vNodes[i].ip, vNodes[i].port, _pool_size, _time_out, "", channel_pool->m_channel_list);
    }
}

bool RedisClient::InitChannel(int32_t idx, const std::string &host, 
                        uint32_t port, uint32_t poolsize, int64_t time_out, const std::string& load_balancer,
                        std::vector<std::shared_ptr<ConnectPool<brpc::Channel>>>& m_channel_list) {
    if (host.empty()) {
        LOG(ERROR) << "Error Host:" << host << " port: " << port;
        return false;
    }

    std::string ip_port = host + ":" + std::to_string(port);

    brpc::ChannelOptions options;
    options.protocol = brpc::PROTOCOL_REDIS;
    options.connection_type = "";
    options.timeout_ms = time_out;
    options.max_retry = 1;
    options.backup_request_ms = -1;
    auto build_connection = [](std::string ip_port, std::string load_balancer, brpc::ChannelOptions options){
        auto channel = std::make_shared<brpc::Channel>();
        if (channel->Init(ip_port.c_str(), load_balancer.c_str(), &options) != 0) {
            LOG(ERROR) << "Fail to initialize channel";
            return std::shared_ptr<brpc::Channel>();
        }
        return channel;
    };
    // LOG(ERROR) << "poolsize: " << poolsize;
    auto conn_pool_ptr = new ConnectPool<brpc::Channel>(poolsize, 100, std::bind(build_connection, ip_port, load_balancer, options));
    std::shared_ptr<ConnectPool<brpc::Channel>> conn_pool;
    conn_pool.reset(conn_pool_ptr);
    m_channel_list[idx] = conn_pool;
    return true;
}

void RedisClient::InitSlotNodeMap(std::vector<int>& slot_ipport_index_vec, const std::vector<NodeInfo>& nodes) {
    slot_ipport_index_vec.resize(16384, -1); // redis slot数为16384, 预分配好，-1 代表slot不存在
    for (size_t i = 0; i < nodes.size(); ++i) {
        const NodeInfo& node = nodes.at(i);
        for (auto iter = node.mSlots.begin(); 
                iter != node.mSlots.end(); ++iter) {
            uint32_t start_slot = iter->first;
            uint32_t end_slot = iter->second;
            for (uint32_t start = start_slot; start <= end_slot; ++start) {
                if (start < slot_ipport_index_vec.size()) {
                    slot_ipport_index_vec[start] = i;
                } else {
                    LOG(ERROR) << "start is greater than 16384, start:" << start;
                    slot_ipport_index_vec.resize(start + 1, -1);
                    slot_ipport_index_vec[start] = i;
                }
            }
        }
    }
}

struct Rbargs {
    Rbargs (std::function<void(void)> fun) : func(fun){}
    std::function<void(void)> func;
};
static void* r_b_func(void * args_tmp) {
    Rbargs* args = (Rbargs*)args_tmp;
    args->func();
    delete args;
    return nullptr;
}
int RedisClient::fetch(std::shared_ptr<graph_frame::Context> context, const std::string& key,
        std::function<void(std::shared_ptr<::google::protobuf::Message>, std::shared_ptr<brpc::Controller> cntl_ptr)> call_back,
        int64_t timeout_ms, int max_retry) {
    std::shared_ptr<brpc::RedisRequest> request = std::make_shared<brpc::RedisRequest>();
    auto response_ptr = std::make_shared<brpc::RedisResponse>();
    auto cntl_ptr = std::make_shared<brpc::Controller>();

    if (timeout_ms > 0) {
        cntl_ptr->set_timeout_ms(timeout_ms);
    }
    if (max_retry > 0) {
        cntl_ptr->set_max_retry(max_retry);
    }

    butil::StringPiece parts[2];
    parts[0].set("GET");
    parts[1].set(key.c_str(), key.size());
    request->AddCommandByComponents(&parts[0], 2);


    auto pool_index_t = pool_index.load(std::memory_order_acquire);
    auto channel_pool_ptr = channel_pool_vec[pool_index_t];
    uint32_t index = channel_pool_ptr->GetKeyNodeIndex(key.c_str(), key.size());
    std::shared_ptr<brpc::Channel> channel = channel_pool_ptr->GetConnection(index);
    if (channel == nullptr) {
        LOG(ERROR) << "get redis channel is nullpr" ;
        return -1;
    }
    std::shared_ptr<::google::protobuf::Message> msg = std::dynamic_pointer_cast<brpc::RedisResponse>(response_ptr);
    auto redis_call_back = brpc::NewCallback(this, &RedisClient::response_handler, cntl_ptr, msg, call_back);
    channel->CallMethod(nullptr, cntl_ptr.get(), request.get(), response_ptr.get(), redis_call_back);
    channel_pool_ptr->FreeConnection(channel, index);
    return 0;
}

int RedisClient::multi_fetch(std::shared_ptr<graph_frame::Context> context, const std::vector<std::string>& data, int start_index, int end_index, std::function<void(std::shared_ptr<::google::protobuf::Message> msg, std::shared_ptr<brpc::Controller> cntl)> call_back, int64_t timeout_ms, int max_retry) {
    std::shared_ptr<brpc::RedisRequest> request = std::make_shared<brpc::RedisRequest>();
    auto response_ptr = std::make_shared<brpc::RedisResponse>();
    auto cntl_ptr = std::make_shared<brpc::Controller>();

    if (timeout_ms > 0) {
        cntl_ptr->set_timeout_ms(timeout_ms);
    }
    if (max_retry > 0) {
        cntl_ptr->set_max_retry(max_retry);
    }

    std::vector<butil::StringPiece> parts;
    parts.resize(std::min(static_cast<size_t>(end_index),data.size()) - start_index + 1);
    parts[0].set("MGET");
    for (int i = start_index; i < data.size() && i < end_index; ++i) {
        const std::string k = data[i];
        parts[i + 1 - start_index].set(k.c_str(), k.size());
    }

    // LOG(ERROR) << "redis MGET part size: " << parts.size() ;
    request->AddCommandByComponents(parts.data(), parts.size());

    auto pool_index_t = pool_index.load(std::memory_order_acquire);
    auto channel_pool_ptr = channel_pool_vec[pool_index_t];
    uint32_t index = channel_pool_ptr->GetKeyNodeIndex(data[start_index].c_str(), data[start_index].size());
    std::shared_ptr<brpc::Channel> channel = channel_pool_ptr->GetConnection(index);

    if (channel == nullptr) {
        LOG(ERROR) << "get redis channel is nullpr" ;
        return -1;
    }
   
    std::shared_ptr<::google::protobuf::Message> msg = std::dynamic_pointer_cast<brpc::RedisResponse>(response_ptr);
    auto redis_call_back = brpc::NewCallback(this, &RedisClient::response_handler, cntl_ptr, msg, call_back);
    channel->CallMethod(nullptr, cntl_ptr.get(), request.get(), response_ptr.get(), redis_call_back);
    channel_pool_ptr->FreeConnection(channel, index);
    return 0;
}

std::shared_ptr<ChannelPool> RedisClient::get_cur_channel_pool() {
    auto pool_index_t = pool_index.load(std::memory_order_acquire);
    auto channel_pool_ptr = channel_pool_vec[pool_index_t];
    return channel_pool_ptr;
}

void RedisClient::response_handler(std::shared_ptr<brpc::Controller> cntl, std::shared_ptr<::google::protobuf::Message> msg, std::function<void(std::shared_ptr<::google::protobuf::Message> msg, std::shared_ptr<brpc::Controller> cntl)> call_back) {
    if (cntl->Failed()) {
        LOG(ERROR) << "cntl failed, remote_ip:" << cntl->remote_side()
                     << " cost_us:" << cntl->latency_us()
                     << " error:"<< cntl->ErrorText() ;
        if (msg != nullptr) {
            msg.reset();
        }
    }

    call_back(msg, cntl);

    return;
}
void RedisClient::result_handler(brpc::Controller* cntl, std::shared_ptr<brpc::RedisResponse> response_ptr,
    std::function<void(std::shared_ptr<brpc::RedisResponse>)> call_back) {
            std::unique_ptr<brpc::Controller> cntl_guard(cntl);
}
inline bool NodeInfo::CheckSlot(uint32_t slot_index) {
    for (auto citer = mSlots.begin(); citer != mSlots.end(); ++citer) {
        if ((slot_index >= citer->first) && (slot_index <= citer->second)) {
            return true;
        }
    }
    return false;
}

inline bool NodeInfo::ParseNodeString(const std::string& nodeString) {
    std::string::size_type ColonPos = nodeString.find(':');
    if (ColonPos == std::string::npos) {
        return false;
    } else {
        const std::string port_str = nodeString.substr(ColonPos + 1);
        port = atoi(port_str.c_str());
        ip = nodeString.substr(0, ColonPos);
        return true;
    }
}

inline void NodeInfo::ParseSlotString(const std::string& SlotString) {
    uint32_t StartSlot = 0;
    uint32_t EndSlot = 0;
    std::string::size_type BarPos = SlotString.find('-');
    if (BarPos == std::string::npos) {
        StartSlot = atoi(SlotString.c_str());
        EndSlot = StartSlot;
    } else {
        const std::string EndSlotStr = SlotString.substr(BarPos + 1);
        EndSlot = atoi(EndSlotStr.c_str());
        StartSlot = atoi(SlotString.substr(0, BarPos).c_str());
    }
    if (StartSlot > 16384 || EndSlot > 16384) {
        LOG(ERROR) << "error, wrong SlotString:" << SlotString.c_str();
    }
    mSlots.push_back(std::make_pair(StartSlot, EndSlot));
}

} // end of namesapce client
