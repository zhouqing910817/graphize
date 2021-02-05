#pragma once
#include <string>
#include <memory>
#include "google/protobuf/message.h"
#include <google/protobuf/util/json_util.h>
#include <set>
#include <vector>
#include <unordered_map>

namespace Rec {
namespace Util {

// get time in seconds
inline double get_cur_time() {
    struct timespec tp;
    clock_gettime(CLOCK_REALTIME, &tp);
    return tp.tv_sec + tp.tv_nsec * 1e-9;
} 
int64_t get_cur_time_us();

std::shared_ptr<google::protobuf::Message> createMessage(const std::string& typeName);

std::string message_to_json(const google::protobuf::Message& message);

}
}
