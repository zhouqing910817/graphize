#pragma once
#include <unordered_map>
#include <memory>
#include <iostream>
#include "butil/logging.h"
#include <iostream>
namespace graph_frame {
class Node;
class GenericServiceContext;
class ServiceFactory {
public:
virtual Node* create() = 0;
virtual GenericServiceContext* create_context() = 0;
};
extern std::unordered_map<std::string, ServiceFactory*> global_service_map;
} // end of namespace

#define SERVICE_REGISTER(ServiceClass) class ServiceClass##Factory : public ::graph_frame::ServiceFactory { \
public: \
::graph_frame::Node* create() override { \
    return new ServiceClass(); \
} \
::graph_frame::GenericServiceContext* create_context() override { \
		auto cxt = new ServiceClass::ServiceClass##Context(); \
		/*LOG(ERROR) << "create service context type: " << typeid(*cxt).name();*/\
		return cxt; \
}\
}; \
class ServiceClass##Register { \
public: \
ServiceClass##Register() { \
    ::graph_frame::global_service_map.insert({#ServiceClass, new ServiceClass##Factory()}); \
	LOG(ERROR) << "register class: " << &::graph_frame::global_service_map << " " << #ServiceClass << "size:" << ::graph_frame::global_service_map.size() << std::endl; \
	std::cout << "register class: " << &::graph_frame::global_service_map << " " << #ServiceClass << "size:" << ::graph_frame::global_service_map.size() << std::endl; \
} \
}; \
static ServiceClass##Register ServiceClass##_inst_;

::graph_frame::ServiceFactory* GET_SERVICE_FACTORY(const std::string & ServiceClassName);
