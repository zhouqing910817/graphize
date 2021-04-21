#include "service/done.h"
#include "butil/time.h"
#include "bthread/id.h"
#include "common/context.h"
#include <gflags/gflags.h>
#include "frame/register.h"
#include <functional>
#include "service/start_service.h"
namespace rec {
namespace service {
SERVICE_REGISTER(DoneService);
void DoneService::init(hocon::shared_config config) {

}
int DoneService::do_service(std::shared_ptr<graph_frame::Context> context) {
	LOG(ERROR) << "do DoneService ...";
	auto start_service_context = StartService::get_const_context(context, context->start_context);
        LOG(ERROR) << "name:" << name << " id:" << id;
	LOG(ERROR) << "done service read request_id:" << start_service_context->request_id << " start_service_context:" << start_service_context;

    return 0;
}
} // end of namespace service
} // end of namespace rec
