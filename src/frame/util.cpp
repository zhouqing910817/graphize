#include "frame/util.h"
#include <time.h>
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
