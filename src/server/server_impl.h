#include <gflags/gflags.h>
#include <butil/logging.h>
#include <brpc/server.h>
#include "proto/echo.pb.h"

namespace example {
class EchoServiceImpl : public EchoService {
public:
    EchoServiceImpl() {};
    virtual ~EchoServiceImpl() {};
    virtual void Echo(google::protobuf::RpcController* cntl_base,
                      const EchoRequest* request,
                      EchoResponse* response,
                      google::protobuf::Closure* done);
};
}  // namespace example

