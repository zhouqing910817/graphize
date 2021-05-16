#include "frame/util.h"
#include <time.h>
#include <boost/algorithm/string.hpp>
#include <glog/logging.h>

namespace Rec {
namespace Util {
std::shared_ptr<google::protobuf::Message> createMessage(const std::string& typeName) {
    google::protobuf::Message* message = nullptr;
    const google::protobuf::Descriptor* descriptor =
        google::protobuf::DescriptorPool::generated_pool()->FindMessageTypeByName(typeName);
    if (!descriptor) {
			std::cout << "createMessage failed, message name: " << typeName << std::endl;
    } else {
        const google::protobuf::Message* prototype = 
            google::protobuf::MessageFactory::generated_factory()->GetPrototype(descriptor);
        if (prototype) {
            message = prototype->New();
        } else {
			std::cout << "createMessage failed, message name: " << typeName << std::endl;
        }
    }
    return std::shared_ptr<google::protobuf::Message>(message);
}

std::shared_ptr<std::vector<int>> getTagSeqByName(google::protobuf::Message* msg, const std::string& name) {
    auto reflection = msg->GetReflection();
    auto descriptor = msg->GetDescriptor();
    std::shared_ptr<std::vector<std::string>> vec = std::make_shared<std::vector<std::string>>();
    boost::split(*vec, name, boost::is_any_of("."), boost::token_compress_on);
    std::shared_ptr<std::vector<int>> tagVec = std::make_shared<std::vector<int>>();
    tagVec->reserve(vec->size());
    for (std::string& fname : *vec) {
        auto field = descriptor->FindFieldByName(fname);
        if (field == nullptr) {
            LOG(FATAL) << "wrong field name:" << fname << " in " << name;
            return tagVec;
        }
        tagVec->push_back(field->index());
    }
    return tagVec;
}

void fillMessage(google::protobuf::Message* msg, const std::vector<int>& tagVec, const std::vector<std::pair<char*, int>>& data) {
    if (data.size() <= 0) {
        return;
    }
    auto reflection = msg->GetReflection();
    auto descriptor = msg->GetDescriptor();
    const google::protobuf::FieldDescriptor* fd = nullptr;
    for (auto i : tagVec) {
        auto f = descriptor->field(i);
        if (f == nullptr) {
            LOG(FATAL) << "wrong index";
            return;
        }
        fd = f;
    }
    if (fd->is_repeated()) {
        auto msgRepeated
            = reflection->MutableRepeatedPtrField<google::protobuf::Message>(msg, fd);
        msgRepeated->Reserve(data.size());
        for (const auto & p : data) {
            auto subMsg = msgRepeated->Add();
            subMsg->ParseFromArray(p.first, p.second);
        }
    } else if (fd->is_map()) {
        LOG(FATAL) << " unexpected field type, just repeated message or message";
    } else if (fd->message_type()) {
        auto subMsg = reflection->MutableMessage(msg, fd);
        subMsg->ParseFromArray(data[0].first, data[0].second);
    } else {
        LOG(FATAL) << " unexpected field type, just repeated message or message";
    }
}

std::string message_to_json(const google::protobuf::Message& message) {
    std::string json_string;
    google::protobuf::util::JsonPrintOptions options;
    options.add_whitespace = true;
    options.always_print_primitive_fields = true;
    options.preserve_proto_field_names = true;
    auto status = google::protobuf::util::MessageToJsonString(message, &json_string, options);
    return json_string;
}

} // Util
} // Rec
