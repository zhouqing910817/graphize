#include "frame/client/redis/channel_pool.h"
#include "butil/logging.h"

namespace redis_client {
uint16_t ChannelPool::crc16(const char *buf, int32_t len) {
    int32_t counter;
    uint16_t crc = 0;
    for (counter = 0; counter < len; counter++)
            crc = (crc<<8) ^ crc16tab[((crc>>8) ^ *buf++)&0x00FF];
    return crc;
}

uint32_t ChannelPool::GetKeyNodeIndex(const char* key, size_t len) {
    auto slot = GetKeySlot(key, len);
    if (slot < slot_ipport_index_vec.size()) {
        int target_index = slot_ipport_index_vec[slot];
        if (target_index == -1) {
            LOG(ERROR) << "slot: " << slot << " is not set" ;
            return 0;
        }
        return target_index;
    }

    return 0;
}

uint32_t ChannelPool::GetKeySlot(const char* key, size_t len) {
    // get slot
    uint32_t slot = -1;
    bool slot_unset = true;
    if (slot_unset && key == nullptr) {
        slot = 0;
        slot_unset = false;
    } 

    size_t s, e; /* start-end indexes of { and } */

    if (slot_unset) {
        for (s = 0; s < len; s++)
            if (key[s] == '{') break;
        /* No '{' ? Hash the whole key. This is the base case. */
        if (s == len) {
            slot = crc16(key, len) & 0x3FFF;
            slot_unset = false;
        }
    }

    if (slot_unset) {
        /* '{' found? Check if we have the corresponding '}'. */
        for (e = s + 1; e < len; e++)
            if (key[e] == '}') break;

        /* No '}' or nothing betweeen {} ? Hash the whole key. */
        if (e == len || e == s + 1) {
            slot = crc16(key, len) & 0x3FFF;
            slot_unset = false;
        }
    }

    if (slot_unset) {
        slot = crc16(key + s + 1, e - s - 1) & 0x3FFF; // 0x3FFF == 16383
        slot_unset = false;
    }

    if (slot_unset) {
        LOG(ERROR) << "can not find a slot, set slot 0" ;
        slot = 0;
        slot_unset = false;
    }
    return slot;
}

std::shared_ptr<brpc::Channel> ChannelPool::GetConnection(uint32_t idx) {
    if (idx >= m_channel_list.size()) {
        LOG(ERROR) << "idx greater than m_channel_list.size(); "
        << "idx:" << idx << " m_channel_list.size:" << m_channel_list.size();
        return std::shared_ptr<brpc::Channel>();
    }
    auto channel = m_channel_list[idx]->borrow();
    if (channel != nullptr) {
    } else {
        LOG(ERROR) << "RedisPool::GetConnection()  error";
    }
    return channel;
}

void ChannelPool::FreeConnection(std::shared_ptr<brpc::Channel> channel, uint32_t index) {
    m_channel_list[index]->release(channel);
}
} // end of namespace client
