#pragma once
#include "frame/graph.h"
#include <vector>
#include <memory>

namespace rec {
namespace service {
class DoneService: public graph_frame::Node {
public:
    virtual void init(hocon::shared_config config) override;
    int do_service(std::shared_ptr<graph_frame::Context> context) override;
    // return value: concurrent num
    const std::string type() override {
        return "cpu";
    }
};
} // end of namespace
} // end of namespace
