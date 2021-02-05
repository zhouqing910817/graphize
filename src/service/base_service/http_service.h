#pragma once
#include <memory>
#include <functional>
#include "service/base_service/http_service.h"
#include "service/base_service/brpc_service.h"
#include "frame/gtransport_manager.h"
#include <brpc/channel.h>
#include "brpc/callback.h"
#include <hocon/config.hpp>

namespace brpc{
class Controller;
}

namespace graph_frame {
class Context;
namespace base {
class HttpService : public BrpcService {
  public:
    int do_service(std::shared_ptr<Context> context) override;
    virtual void init(hocon::shared_config conf) {
    }
    virtual int process_response(std::shared_ptr<Context> context, brpc::Controller* cntl) = 0;

    void service_cb(brpc::Controller* cntl, google::protobuf::Message* respons, std::shared_ptr<Context> context);
    virtual void make_uri(std::shared_ptr<Context> context, std::string* uri) = 0;
  protected:
    std::string _transport_name;
    std::string _uri;
};
} // end of namespace base
} // end of namespace graph_frame
