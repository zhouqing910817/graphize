#pragma once
#include <atomic>
#include <unordered_map>
#include <vector>
#include <functional>
#include <memory>
#include "bthread/bthread.h"
#include "bthread/id.h"
#include  <queue>
#include <typeinfo>
#include <vector>
#include <set>
#include <brpc/channel.h>
#include "brpc/callback.h"
#include "frame/graph_context.h"
#include "frame/register.h"
#include "common/context.h"
#include <hocon/config.hpp>

namespace graph_frame {
DECLARE_bool(run_graph_debug);
class Node;

struct Bargs {
    Bargs(Node* n, std::shared_ptr<graph_frame::Context> ctx) : node(n), context(ctx) {
    }
    Node* node;
    std::shared_ptr<graph_frame::Context> context;
};
class Node {
    public:
	#define GET_OWN_CONTEXT() get_own_context(context, name);
    Node(const std::string& name = "") : name(name){
    }
    virtual void init(hocon::shared_config conf);
    void run(std::shared_ptr<graph_frame::Context> context);
    virtual int do_service(std::shared_ptr<graph_frame::Context> context);
    virtual bool skip(std::shared_ptr<graph_frame::Context> context);
    bool notify(std::shared_ptr<graph_frame::Context> context);

    const std::vector<Node*>& get_output_nodes() {
        return out_nodes;
    }
    void set_name(const std::string& name) {
        this->name = name;
    }

    const std::string& get_name() {
        return name;
    }

    void add_output(Node * node) {
        out_nodes.push_back(node);
        node->incr_input_num();
    }
    size_t get_out_nodes_size() {
        return out_nodes.size();
    }
    void run_output_nodes_if_ready(std::shared_ptr<graph_frame::Context> context);

    void incr_input_num() {
        input_num++;
    }

    int get_input_num() {
        return input_num;
    }
    virtual const std::string type() {
        return "cpu";
    }
    private:
    int input_num = 0;
    std::vector<Node*> out_nodes;
	public:
    std::string name;
    

};

class Graph {
  private:
  Node* root_node = nullptr;
  std::vector<Node*> all_nodes;
  std::string name = "graph_default_name";
  public:
  static Graph& instance() {
      static Graph g;
      return g;
  }
  void set_name(const std::string& name) {
      this->name = name;
  }
  const std::string& get_name() const {
      return name;
  }
  int init(const std::string& path) {
      return 0;
  }
  int init(Node* node, const std::string& name = "") {
      if (!name.empty()) {
          this->name = name;
      }
      root_node = node;
      all_nodes.clear();
      auto cur_node = root_node;
      std::queue<Node*> q;
      q.push(cur_node);
      std::set<Node*> set;
      while(!q.empty()) {
          cur_node = q.front();
          q.pop();
          if (set.find(cur_node) == set.end()) {
              set.insert(cur_node);
              all_nodes.push_back(cur_node);
          }
          for (auto node : cur_node->get_output_nodes()) {
              q.push(node);
          }
      }
      std::cout <<"graph: " << name << " all_nodes size: " << all_nodes.size() << std::endl;
      return 0;
  }
  template<typename Context, class... Args>
  int run(Args... args) {
      // 1. 构造图的上下文
      auto context = std::make_shared<Context>(std::forward<Args>(args)...);
      if (root_node == nullptr) {
          LOG(ERROR) << "root_node is nullptr";
          return -1;
      }
      for (auto node : all_nodes) {
          auto it = context->node_input_num_map.find(node);
          if (it == context->node_input_num_map.end()) {
              context->node_input_num_map[node] = std::make_shared<std::atomic<int>>(node->get_input_num());
			  if (FLAGS_run_graph_debug) {
                  LOG(ERROR) << "node: " << node->get_name() << " input_num:" << context->node_input_num_map[node]->load();
			  }
          }
      }
      // 2. run
      root_node->run(context);
      return 0;
  }
};
#define DEFINE_SERVICE_CONTEXT(ChildServiceContext) class ChildServiceContext; \
	public: \
    static const ChildServiceContext* get_const_context(std::shared_ptr<graph_frame::GraphContext> g_context, const std::string& node_name){ \
        return (const ChildServiceContext*)(g_context->get_context(node_name).get()); \
     } \
    private: \
    static ChildServiceContext* get_own_context(std::shared_ptr<graph_frame::GraphContext> g_context, const std::string& node_name) {\
		auto cxt = g_context->get_context(node_name); \
		/* LOG(ERROR) << "node_name:" << node_name << " get context: " << cxt.get() << "type: " << typeid(*cxt).name();*/\
        return (ChildServiceContext*)(cxt.get());\
    }\
    public:\
    class  ChildServiceContext : public graph_frame::GenericServiceContext 
} // end of namespace vv::frame
