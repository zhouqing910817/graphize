#include "frame/register.h"

namespace graph_frame {

class ServiceFactory;

std::unordered_map<std::string, ServiceFactory*> global_service_map;
} // end of namespace
::graph_frame::ServiceFactory* GET_SERVICE_FACTORY(const std::string & ServiceClassName) {
	LOG(INFO) << "global_service_map addr: " << &::graph_frame::global_service_map << " ServiceClassName: " << ServiceClassName
		   	<< " ::graph_frame::global_service_map size: " << ::graph_frame::global_service_map.size() << std::endl;
	return ::graph_frame::global_service_map[ServiceClassName];
}
