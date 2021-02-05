#include "unittest/graph_frame/src/start_service.h"
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
	func = config->get_string("func");

}
int StartService::do_service(std::shared_ptr<graph_frame::Context> context) {
	LOG(ERROR) << "do service : " << func;
    return 0;
}
} // end of namespace service
} // end of namespace rec
