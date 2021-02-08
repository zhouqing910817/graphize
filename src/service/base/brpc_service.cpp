#include "service/base/brpc_service.h"
#include "common/context.h"
#include <functional>
#include "common/context.h"
#include "frame/util.h"

namespace graph_frame {
namespace base {
using namespace std::placeholders;
void BrpcService::service_cb(brpc::Controller* cntl, google::protobuf::Message* response, std::shared_ptr<Context> context) {
        process_response(context, cntl, response);
		delete cntl;
		delete response;
        run_output_nodes_if_ready(context);
}
int BrpcService::do_service(std::shared_ptr<Context> context) {
    auto s_cb = std::bind(&BrpcService::service_cb, this, std::placeholders::_1, std::placeholders::_2, context);
    // LOG(WARNING) << "_transport_name: " << _transport_name;
    auto channel = GtransportManager::instance().get_transport(_transport_name);
    if (!channel) {
        LOG(WARNING) << "[" << _transport_name << "] channel is nullptr";
        run_output_nodes_if_ready(context);
        return -1;
    }
    call_rpc_method(channel, s_cb, context);
    return 0;
}


int BrpcService::call_rpc_method(std::shared_ptr<brpc::Channel> channel,
    std::function<void(brpc::Controller* cntl, google::protobuf::Message* response)> service_cb,
    std::shared_ptr<Context> context) {
    return 0;
}

google::protobuf::Message* BrpcService::make_request(std::shared_ptr<Context> context) {
    return nullptr;
}

int BrpcService::process_response(std::shared_ptr<Context> context, brpc::Controller* cntl, google::protobuf::Message* response) {
    return 0;
}


} // end of namespace base
} // end of namespace graph_frame
