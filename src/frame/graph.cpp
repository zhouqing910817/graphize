#include "frame/graph.h"
#include "common/context.h"
namespace graph_frame {
DEFINE_bool(run_graph_debug, false, "debug if the graph run is configured");
static void* b_func(void * args_tmp) {
    Bargs* args = (Bargs*)args_tmp;
	if (FLAGS_run_graph_debug) {
        LOG(ERROR) << " bthread run output node: " << args->node->get_name().c_str();
	}
    args->node->run(args->context);
    delete args;
    return nullptr;
}
void Node::run(std::shared_ptr<graph_frame::Context> context) {
    if (!skip(context)) {
	    if (FLAGS_run_graph_debug) {
            LOG(ERROR) << " bthread run output node: " << name;
		}
        do_service(context);
        // io nodes call run_output_nodes_if_ready themselves
        if (type() == std::string("cpu")) {
            run_output_nodes_if_ready(context);
        }
    } else {
        if (FLAGS_run_graph_debug) {
            LOG(ERROR) << "skip " << name;
		}
        run_output_nodes_if_ready(context);
    }
}
int Node::do_service(std::shared_ptr<graph_frame::Context> context) {
    // VLOG_APP(WARNING) << name << " do service; input_num:  " << input_num << " out_nodes size:" << out_nodes.size() ;
    return 0;
}
bool Node::skip(std::shared_ptr<graph_frame::Context> context) {
    return false;
}
bool Node::notify(std::shared_ptr<graph_frame::Context> context) {
    auto it = context->node_input_num_map.find(this);
    if (it == context->node_input_num_map.end()) {
		LOG(ERROR) << "node:" << this << " not found in node_input_num_map, this should not happen";
        return false;
    }
    auto input_num = it->second;
    int old_value = input_num->fetch_sub(1);
	if (FLAGS_run_graph_debug) {
        LOG(ERROR) << name << " notified; " << " old_value: " << old_value;
	}
    if (old_value == 1) {
        return true;
    } else if (old_value <= 0) {
		if (FLAGS_run_graph_debug) {
		    LOG(ERROR) << "too many notify, old_value: " << old_value << ", this should not happen";
		}
    } 
    return false;
}
void Node::run_output_nodes_if_ready(std::shared_ptr<graph_frame::Context> context) {
    int ready_nodes_num = 0;
    Node* last_ready_node = nullptr;
    for (size_t i = 0; i < out_nodes.size(); i++) {
        auto output_node = out_nodes[i];
		if (FLAGS_run_graph_debug) {
            LOG(ERROR) << name << " notify " << output_node->get_name();
		}
        auto ready = output_node->notify(context);
        if (ready) {
			if (FLAGS_run_graph_debug) {
                LOG(ERROR) << output_node->get_name() << " is ready";
			}
            ++ready_nodes_num;
            if (last_ready_node != nullptr) {
                bthread_t tid;
                Bargs* args = new Bargs(last_ready_node, context);
                bthread_start_background(&tid, &BTHREAD_ATTR_SMALL, b_func, args);
            }
            last_ready_node = output_node;

        }
    }
    if (last_ready_node != nullptr) {
        last_ready_node->run(context);
    }
    return;
}
}
