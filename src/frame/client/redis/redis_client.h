#pragma once

#include <brpc/redis.h>
#include <brpc/channel.h>
#include <unordered_map>
#include <memory>
#include <list>
#include "frame/client/redis/channel_pool.h"
#include <thread>
#include "common/context.h"

namespace redis_client {
struct NodeInfo {
    public:
        bool CheckSlot(uint32_t slotindex);
        bool ParseNodeString(const std::string &nodeString);
        void ParseSlotString(const std::string &SlotString);
    public:
        std::string strinfo;
        // The node ID, a 40 characters random string generated when a node is created and never changed again (unless CLUSTER RESET HARD is used)
        std::string id;        
        // The node IP     
        std::string ip;    
        // The node port
        uint16_t port;   
        // A list of comma separated flags: myself, master, slave, fail?, fail, handshake, noaddr, noflags
        std::string flags;  
        bool is_fail; 
        // true if node is master, false if node is salve
        bool is_master;     
        bool is_slave;
        // The replication master
        std::string master_id; 
        int32_t ping_sent;    
        // Milliseconds unix time the last pong was received
        int32_t pong_recv;      
        // The configuration epoch (or version) of the current node (or of the current master if the node is a slave).
        // Each time there is a failover, a new, unique, monotonically increasing configuration epoch is created.
        // If multiple nodes claim to serve the same hash slots, the one with higher configuration epoch wins
        int32_t epoch;         
        // The state of the link used for the node-to-node cluster bus   
        bool connected;     
        // A hash slot number or range
        std::vector<std::pair<uint32_t, uint32_t> > mSlots;
};

class RedisClient {
public:
    RedisClient(const std::string& url, const std::string& lb,
                uint64_t time_out, int pool_size, int max_pool_size, int max_retry = 0) noexcept;
    int fetch(std::shared_ptr<graph_frame::Context> context, const std::string& key,
        std::function<void(std::shared_ptr<::google::protobuf::Message>, std::shared_ptr<brpc::Controller> cntl_ptr)> call_back,
        int64_t timeout_ms = -1, int max_retry = -1);
    int multi_fetch(std::shared_ptr<graph_frame::Context> context, const std::vector<std::string>& data, int start_index, int end_index, std::function<void(std::shared_ptr<::google::protobuf::Message> msg, std::shared_ptr<brpc::Controller> cntl)> call_back, int64_t timeout_ms = -1, int max_retry = -1);
    virtual ~RedisClient() {
    }
    void response_handler(std::shared_ptr<brpc::Controller> cntl, std::shared_ptr<::google::protobuf::Message> msg, std::function<void(std::shared_ptr<::google::protobuf::Message> msg, std::shared_ptr<brpc::Controller> cntl)> call_back);
    // auto latency = -a start;
    void update_channel_pool();
    std::shared_ptr<ChannelPool> get_cur_channel_pool();
private:
    std::string _url;
    std::string _lb;
    uint64_t _time_out;
    int _max_retry;
    int _pool_size;
    std::shared_ptr<std::thread> thread_ptr;
    bool is_channel_pool_changed();

    void init_channel_pool(std::shared_ptr<ChannelPool> channel_pool);
    void InitSlotNodeMap(std::vector<int>& slot_ipport_index_vec, const std::vector<NodeInfo>& nodes);
    bool InitChannel(int32_t idx, const std::string &host, 
                        uint32_t port, uint32_t poolsize, int64_t timeout_ms, const std::string& lb,
                        std::vector<std::shared_ptr<ConnectPool<brpc::Channel>>>& m_channel_list);
    void result_handler(brpc::Controller* cntl, std::shared_ptr<brpc::RedisResponse> response_ptr,
        std::function<void(std::shared_ptr<brpc::RedisResponse>)> call_back);

    std::vector<std::shared_ptr<ChannelPool>> channel_pool_vec;
    std::atomic<int> pool_index;

};

} // end of namespace client
