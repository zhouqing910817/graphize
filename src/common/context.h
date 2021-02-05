#pragma once
#include <mutex>
#include <atomic>
#include <brpc/channel.h>
#include <unordered_set>
#include <brpc/redis.h>
#include "google/protobuf/message.h"
#include "frame/graph_context.h"
#include <unordered_set>
#include <unordered_map>

namespace graph_frame {
    extern std::vector<std::string> global_channel_name_vec;
}
namespace graph_frame {
using namespace google;

class Context : public graph_frame::GraphContext {
public:
    Context();
    virtual ~Context();
    int Init();
};

}
