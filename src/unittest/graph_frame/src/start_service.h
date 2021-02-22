#pragma once
#include "frame/graph.h"
#include <vector>
#include <memory>

namespace rec {
namespace service {
class UtStartService: public graph_frame::Node {
public:
    virtual void init(hocon::shared_config config) override;
    int do_service(std::shared_ptr<graph_frame::Context> context) override;
    const std::string type() override {
        return "cpu";
    }
private:
	// 进程级别数据
	std::string func;

public:
	//请求级别数据
	DEFINE_SERVICE_CONTEXT(UtStartServiceContext) {
		public:
		std::string request_id;
	};
};
} // end of namespace
} // end of namespace
