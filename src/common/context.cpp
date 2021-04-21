#include "common/context.h"
namespace graph_frame {

using namespace google;

Context::Context(const example::EchoRequest* req, example::EchoResponse* resp, brpc::Controller* cntl, google::protobuf::Closure* closure):GraphContext("", req, resp, cntl, closure){
}

Context::~Context() 
{
}

} // end of namespace graph_frame
