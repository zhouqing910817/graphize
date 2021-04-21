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
#include "echo.pb.h"

namespace graph_frame {
using namespace google;

class Context : public graph_frame::GraphContext<example::EchoRequest, example::EchoResponse> {
public:
    Context(const example::EchoRequest* req, example::EchoResponse* resp, brpc::Controller* cntl, google::protobuf::Closure* closure);
    virtual ~Context();
public:
    int start_context;
    int mid1_context;
    int mid2_context;
    int done_context;
};

}
