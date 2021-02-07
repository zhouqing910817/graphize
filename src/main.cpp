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
bool init_graph() {
	// redis_client::RedisClientManager::instance().init("");
	// graph_frame::GtransportManager::instance().init("conf/downstream.conf");
	graph_frame::NodeManager::instance().init("conf/node_service.conf");
	graph_frame::GraphManager::instance().init("conf/graph.conf");

}
int main(int argc, char* argv[]) {
	gflags::ParseCommandLineFlags(&argc, &argv, true);
	std::cout << "start init graph" << std::endl;
	init_graph();
	graph_frame::GraphManager::instance().get_graph("graph1")->run<graph_frame::Context>("graph1");
	sleep(1);
    return 0;
}
