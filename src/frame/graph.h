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
class ServiceFactory; // defined in register.h
class Graph;
class Node {
    public:
    friend class GraphManager;
    friend class Graph;
    public:
	#define GET_OWN_CONTEXT() get_own_context(context, id);
    Node(const std::string& name = "") : name(name){
    }
    virtual int do_service(std::shared_ptr<graph_frame::Context> context);
    virtual bool skip(std::shared_ptr<graph_frame::Context> context);
    virtual void init(hocon::shared_config conf) = 0;

    void run(std::shared_ptr<graph_frame::Context> context);
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
		node->input_num_4_check++;
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
    int input_num_4_check = 0;
    private:
    ServiceFactory* service_context_factory;
    int input_num = 0;
    std::vector<Node*> out_nodes;
    public:
    int id;
    std::string name;
};

class Graph {
  public:
  int max_node_id;
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
      std::stringstream ss;
      ss << "/********* graph: " << name << " *********/" << std::endl;
      while(!q.empty()) {
          cur_node = q.front();
          q.pop();
          if (set.find(cur_node) == set.end()) {
              set.insert(cur_node);
              all_nodes.push_back(cur_node);
          } else {
		      continue;
		  }
		  if (cur_node->get_output_nodes().size() == 0) {
		      ss << cur_node->get_name() << " => NIL" << std::endl;
		  }
          for (auto node : cur_node->get_output_nodes()) {
		      ss << cur_node->get_name() << " => " << node->get_name() << std::endl;
              q.push(node);
          }
      }
      ss << "/********* graph: " << name << " end *********/" << std::endl;
      LOG(ERROR) <<"graph: " << name << " all_nodes size: " << all_nodes.size();
      LOG(ERROR) << "\n" <<  ss.str();
      bool valid = check();
	  if (valid) {
		  return 0;
	  } else {
          return -1;
	  }
  }
  bool check() {
	  std::unordered_map<Node*, bool> del_flag_map;
	  for(auto* node : all_nodes) {
		 del_flag_map[node] = false;
	  }
	  std::list<Node*> zero_input_node;
	  if (root_node && root_node->input_num_4_check == 0) {
	      zero_input_node.push_back(root_node);
	  } else {
		  LOG(FATAL) << "root node is nullptr or input_num != 0";
		  return false;
	  }
	  while(zero_input_node.size() > 0) {
	      auto r = zero_input_node.front();
		  zero_input_node.pop_front();
		  del_flag_map[r] = true; // delete node whose input_num = 0
		  for (auto * n : r->get_output_nodes()) {
		      n->input_num_4_check--;
			  if (n->input_num_4_check == 0) {
				  zero_input_node.push_back(n);
			  }
		  }
	  }
	  std::stringstream ss;
	  for (auto pair : del_flag_map) {
          if (pair.second == false) {
			  ss << pair.first->get_name() << ", ";
		  }
	  }
	  std::string cycle_dependcy_nodes_str = ss.str();
	  if (cycle_dependcy_nodes_str.size() > 0) {
		  LOG(FATAL) << "[" << cycle_dependcy_nodes_str.substr(0, cycle_dependcy_nodes_str.size() - 2) << "] has cycle dependcy";
		  return false;
	  }
	  return true;
  }
  template<typename Context, class... Args>
  int run(Args... args) {
      // 1. 构造图的上下文
      auto context = std::make_shared<Context>(std::forward<Args>(args)...);
      context->set_graph_name(name);
      if (root_node == nullptr) {
          LOG(ERROR) << "root_node is nullptr";
          return -1;
      }
      if (all_nodes.size() <= 0) {
          LOG(FATAL) << "nodes num <=0, graph name:" << name;
      }
      context->node_input_num_map.resize(all_nodes.size());
      context->node_service_context_vec.resize(max_node_id + 1);
      for (auto node : all_nodes) {
          if (node->id >= context->node_input_num_map.size()) {
              LOG(FATAL) << "error node->id:" << node->id << " >= node_input_num_map.size:" << context->node_input_num_map.size();
          }
          context->node_input_num_map[node->id] = std::make_shared<std::atomic<int>>(node->get_input_num());
          if (FLAGS_run_graph_debug) {
              LOG(ERROR) << "node: " << node->get_name() << " input_num:" << context->node_input_num_map[node->id]->load();
          }
          context->node_service_context_vec[node->id].reset(node->service_context_factory->create_context());
          LOG(ERROR) << "create context for node id:" << node->id << " node_service_context:" << context->node_service_context_vec[node->id].get();
      }
      // 2. run
      root_node->run(context);
      return 0;
  }
};
#define DEFINE_SERVICE_CONTEXT(ChildServiceContext) class ChildServiceContext; \
	public: \
    static const ChildServiceContext* get_const_context(std::shared_ptr<graph_frame::Context> g_context, int id){ \
        return (const ChildServiceContext*)(g_context->get_context(id).get()); \
     } \
    private: \
    static ChildServiceContext* get_own_context(std::shared_ptr<graph_frame::Context> g_context, int id) {\
		auto cxt = g_context->get_context(id); \
		/* LOG(ERROR) << "node_name:" << node_name << " get context: " << cxt.get() << "type: " << typeid(*cxt).name();*/\
        return (ChildServiceContext*)(cxt.get());\
    }\
    public:\
    class  ChildServiceContext : public graph_frame::GenericServiceContext 
} // end of namespace vv::frame
