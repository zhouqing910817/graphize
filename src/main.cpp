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
#include <boost/filesystem.hpp>

bool init_graph(const std::string& graph_frame_file) {
    return graph_frame::init_graph_frame(graph_frame_file);
}


void init_glog(const char* app_name, const std::string& log_path) {
    using namespace google;
    FLAGS_logtostderr = 0;
    google::InitGoogleLogging(app_name);
    FLAGS_log_dir = "./log/";
    google::SetLogDestination(ERROR, (FLAGS_log_dir + std::string(app_name) + std::string(".error")).c_str());
    google::SetLogDestination(INFO, (FLAGS_log_dir + std::string(app_name) + std::string(".info")).c_str());
    google::SetLogDestination(WARNING, (FLAGS_log_dir + std::string(app_name) + std::string(".warning")).c_str());
}
int main(int argc, char* argv[]) {
	gflags::ParseCommandLineFlags(&argc, &argv, true);
    std::string server_conf_file = "conf/server.conf";

    hocon::shared_config root_conf = hocon::config::parse_file_any_syntax(server_conf_file);
    hocon::shared_object root_obj = root_conf->root();
    std::string log_path = root_conf->get_string("log_path");
    bool suc = init_graph(server_conf_file);
    if (!suc) {
        return -1;
    }

    FLAGS_logtostderr = 1;
    FLAGS_log_dir = "./log";
    if(!boost::filesystem::exists(log_path)) {
        boost::filesystem::create_directories(log_path);
    }

	init_glog(argv[0], log_path);
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
    auto p = root_conf->get_int("cores_multi");
    if (p == 0) {
        LOG(ERROR) << "cores_multi should not be 0";
    }
    // Start the server.
    brpc::ServerOptions options;
    options.idle_timeout_sec = 600;
    options.num_threads = std::thread::hardware_concurrency() * p;
    if (server.Start(port, &options) != 0) {
        LOG(ERROR) << "Fail to start EchoServer";
        return -1;
    }

    // Wait until Ctrl-C is pressed, then Stop() and Join() the server.
    server.RunUntilAskedToQuit();
    
    return 0;
}
