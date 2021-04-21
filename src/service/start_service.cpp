#include "service/start_service.h"
#include "butil/time.h"
#include "bthread/id.h"
#include "common/context.h"
#include <gflags/gflags.h>
#include "frame/register.h"
#include <functional>
namespace rec {
namespace service {
SERVICE_REGISTER(StartService);
void StartService::init(hocon::shared_config config) {
	//初始化配置文件，进程级别数据 
	func = config->get_string("func");

}
int StartService::do_service(std::shared_ptr<graph_frame::Context> context) {
        if (name == "start") {
            context->start_context = id;
        }
	LOG(ERROR) << "do service : " << func << " node_name: " << name;
        LOG(ERROR) << "name:" << name << " id:" << id;
	auto start_service_context = GET_OWN_CONTEXT();
	// LOG(ERROR) << "start_service_context: " << start_service_context;
        if (name == "start") {
	    start_service_context->request_id = name + " set request_id";
        }
        LOG(ERROR) << "write start_service_context:" << start_service_context;
    return 0;
}
} // end of namespace service
} // end of namespace rec
