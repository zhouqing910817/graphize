#pragma once
#include <memory>
#include <functional>
#include "frame/gtransport_manager.h"
#include <brpc/channel.h>
#include "brpc/callback.h"
#include "butil/logging.h"

namespace brpc{
class Controller;
}

namespace graph_frame {

template<typename Response>
void handle_response(brpc::Controller* cntl, Response* response, std::function<void(brpc::Controller*, google::protobuf::Message*)> service_cb) {
    if (cntl->Failed()) {
        LOG(WARNING) << "rpc failed, " << cntl->ErrorText();
    }
    // LOG(WARNING) << "Received response from " << cntl->remote_side()
    //     << " latency=" << cntl->latency_us() << "us";
    service_cb(cntl, (google::protobuf::Message*)response);
}

template <typename Request, typename Response, typename Stub>
void call_method(void (Stub::*method)(::google::protobuf::RpcController*, Request*, Response*, google::protobuf::Closure*),
    google::protobuf::Message* request_msg,
    std::shared_ptr<brpc::Channel> channel, std::function<void(brpc::Controller*, google::protobuf::Message*)> cb) {
    if (!channel) {
        LOG(WARNING) << " channel is nullptr";
        return;
    }
    auto cntl = new brpc::Controller();
    Request* request = (Request*)request_msg;
    auto response = new Response;
    Stub stub(channel.get());
    google::protobuf::Closure* done = brpc::NewCallback(
                &handle_response<Response>, cntl, response, cb);
    (stub.*method)((::google::protobuf::RpcController*)(cntl), request, response, done);
}


} // end of namespace graph_frame
