#include "service/base/http_service.h"
#include "common/context.h"
#include <functional>
#include "common/context.h"
#include "frame/call_mothod_util.h"

namespace graph_frame {
namespace base {
using namespace std::placeholders;
void HttpService::service_cb(brpc::Controller* cntl, google::protobuf::Message* response, std::shared_ptr<Context> context) {
        this->process_response(context, cntl);
		delete cntl;
        run_output_nodes_if_ready(context);
}
int HttpService::do_service(std::shared_ptr<Context> context) {
    auto channel = GtransportManager::instance().get_transport(_transport_name);
    if (!channel) {
        LOG(ERROR) << _transport_name << " channel is nullptr";
        run_output_nodes_if_ready(context);
        return -1;
    }

    auto cntl = new brpc::Controller;
    std::string uri = "";
    make_uri(context, &uri);
    cntl->http_request().uri() = uri;
    cntl->http_request().set_method(brpc::HTTP_METHOD_GET);
    google::protobuf::Closure* done = brpc::NewCallback(this, &HttpService::service_cb, cntl, (google::protobuf::Message*)nullptr, context);
    channel->CallMethod(NULL, cntl, NULL, NULL, done);
    return 0;
}

} // end of namespace base
} // end of namespace graph_frame
