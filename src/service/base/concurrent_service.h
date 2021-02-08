#pragma once
#include "frame/graph.h"
#include <vector>
#include <memory>
#include "hocon/config.hpp"

namespace rec {
namespace service {
class ConcurrentService: public graph_frame::Node {
public:
    virtual void init(hocon::shared_config conf) override;
    int do_service(std::shared_ptr<graph_frame::Context> context) override;
    // return value: concurrent num
    virtual size_t prepare_sub_requests(std::shared_ptr<graph_frame::Context> context) ;
    virtual void process(std::shared_ptr<graph_frame::Context> context, size_t index);
    virtual void merge(std::shared_ptr<graph_frame::Context> context, size_t slice_num);
    const std::string type() override {
        return "io";
    }
};
} // end of namespace start
} // end of namespace graph_frame
