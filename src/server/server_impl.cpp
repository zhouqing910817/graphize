#include "server/server_impl.h"
#include "frame/graph_manager.h"
#include "frame/graph_context.h"
#include "frame/graph.h"
#include "brpc/controller.h"

namespace example {
void EchoServiceImpl::Echo(google::protobuf::RpcController* cntl_base,
                  const EchoRequest* request,
                  EchoResponse* response,
                  google::protobuf::Closure* done) {
    // This object helps you to call done->Run() in RAII style. If you need
    // to process the request asynchronously, pass done_guard.release().
    brpc::ClosureGuard done_guard(done);

    brpc::Controller* cntl =
        static_cast<brpc::Controller*>(cntl_base);

    // The purpose of following logs is to help you to understand
    // how clients interact with servers more intuitively. You should 
    // remove these logs in performance-sensitive servers.
    LOG(INFO) << "Received request[log_id=" << cntl->log_id() 
              << "] from " << cntl->remote_side() 
              << " to " << cntl->local_side()
              << ": " << request->message()
              << " (attached=" << cntl->request_attachment() << ")";
    std::string graph_name = "graph1";
    graph_frame::GraphManager::instance().get_graph(graph_name)->run<graph_frame::Context>(request, response, cntl, done);
    done_guard.release();

}
}  // namespace example
