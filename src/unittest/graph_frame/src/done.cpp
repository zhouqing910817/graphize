#include "unittest/graph_frame/src/done.h"
#include "butil/time.h"
#include "bthread/id.h"
#include "common/context.h"
#include <gflags/gflags.h>
#include "frame/register.h"
#include <functional>
namespace rec {
namespace service {
SERVICE_REGISTER(DoneService);
void DoneService::init(hocon::shared_config config) {

}
int DoneService::do_service(std::shared_ptr<graph_frame::Context> context) {
	LOG(ERROR) << "do DoneService";
    return 0;
}
} // end of namespace service
} // end of namespace rec
