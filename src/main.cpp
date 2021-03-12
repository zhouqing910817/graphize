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
#include <brpc/server.h>
#include "echo.pb.h"
#include "server/server_impl.h"

bool init_graph(const std::string& graph_frame_file) {
    return graph_frame::init_graph_frame(graph_frame_file);
}

void init_glog(const std::string& log_path) {

// 	using namespace logging;
// 
//     ::logging::LoggingSettings log_setting;  // 创建LoggingSetting对象进行设置
//     log_setting.log_file = log_path.c_str(); // 设置日志路径
//     log_setting.logging_dest = logging::LOG_TO_FILE; // 设置日志写到文件，不写的话不生效
//     ::logging::InitLogging(log_setting);     // 应用日志设置
}
int main(int argc, char* argv[]) {
	gflags::ParseCommandLineFlags(&argc, &argv, true);
	LOG(ERROR) << "start init graph" << std::endl;
    std::string server_conf_file = "conf/server.conf";

	hocon::shared_config root_conf = hocon::config::parse_file_any_syntax(server_conf_file);
	hocon::shared_object root_obj = root_conf->root();
    std::string log_path = root_conf->get_string("log_path");
	init_glog(log_path);
	bool suc = init_graph("conf/server.conf");
	if (!suc) {
		return -1;
	}
    int port = root_conf->get_int("port");
    
    // Generally you only need one Server.
    brpc::Server server;

    // Instance of your service.
    example::EchoServiceImpl echo_service_impl;

    // Add the service into server. Notice the second parameter, because the
    // service is put on stack, we don't want server to delete it, otherwise
    // use brpc::SERVER_OWNS_SERVICE.
    if (server.AddService(&echo_service_impl,
                          brpc::SERVER_DOESNT_OWN_SERVICE) != 0) {
        LOG(ERROR) << "Fail to add service";
        return -1;
    }

    // Start the server.
    brpc::ServerOptions options;
    options.idle_timeout_sec = 600;
    if (server.Start(port, &options) != 0) {
        LOG(ERROR) << "Fail to start EchoServer";
        return -1;
    }

    // Wait until Ctrl-C is pressed, then Stop() and Join() the server.
    server.RunUntilAskedToQuit();
    
    return 0;
}
