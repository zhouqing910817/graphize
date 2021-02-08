#include <iostream>
#include "frame/graph.h"
#include "butil/logging.h"
#include "frame/client/redis/redis_client_manager.h"
#include "frame/graph_manager.h"
#include "frame/gtransport_manager.h"
#include "frame/graph_manager.h"
#include "frame/node_manager.h"
#include "frame/node_manager.h"
#include "common/context.h"
#include "butil/logging.h"
bool init_graph() {
	// redis_client::RedisClientManager::instance().init("");
	// graph_frame::GtransportManager::instance().init("conf/downstream.conf");
	return graph_frame::NodeManager::instance().init("conf/node_service.conf") &&
	graph_frame::GraphManager::instance().init("conf/graph.conf");

}

void init_glog() {
	using namespace logging;
	std::string log_path = "log/graphize.log";

    ::logging::LoggingSettings log_setting;  // 创建LoggingSetting对象进行设置
    log_setting.log_file = log_path.c_str(); // 设置日志路径
    log_setting.logging_dest = logging::LOG_TO_FILE; // 设置日志写到文件，不写的话不生效
    ::logging::InitLogging(log_setting);     // 应用日志设置
}
int main(int argc, char* argv[]) {
	gflags::ParseCommandLineFlags(&argc, &argv, true);
	LOG(ERROR) << "start init graph" << std::endl;
	init_glog();
	bool suc = init_graph();
	if (!suc) {
		return -1;
	}
	graph_frame::GraphManager::instance().get_graph("graph1")->run<graph_frame::Context>("graph1");
	sleep(1);
    return 0;
}
