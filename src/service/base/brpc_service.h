#pragma once
#include <memory>
#include <functional>
#include "frame/gtransport_manager.h"
#include <brpc/channel.h>
#include "brpc/callback.h"
#include "frame/graph.h"
#include <hocon/config.hpp>
namespace google {
namespace protobuf {
	class Message;
}
}
namespace brpc{
class Controller;
}

namespace graph_frame {
class Context;
namespace base {
class BrpcService : public Node {
  public:
    virtual int do_service(std::shared_ptr<Context> context) override;
    virtual void init(hocon::shared_config conf){
    }
    virtual int call_rpc_method(std::shared_ptr<brpc::Channel> channel,
        std::function<void(brpc::Controller* cntl, google::protobuf::Message* response)> service_cb,
        std::shared_ptr<Context> context);
    virtual int process_response(std::shared_ptr<Context> context, brpc::Controller* cntl, google::protobuf::Message* response);
    virtual google::protobuf::Message* make_request(std::shared_ptr<Context> context);
    const std::string type() override {
        return "io";
    }
    void service_cb(brpc::Controller* cntl, google::protobuf::Message* respons, std::shared_ptr<Context> context);
  protected:
    std::string _transport_name;
};
} // end of namespace base
} // end of namespace graph_frame
