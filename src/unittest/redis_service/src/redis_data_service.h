#include "service/base/redis_service.h"

namespace service {
using namespace graph_frame;
class RedisDataService : public graph_frame::RedisService {
public:
	RedisDataService(){}
	void init(hocon::shared_config conf) override;
	void process_response(std::shared_ptr<Context> context, const RedisResult& redis_result) override;
public:
	//请求级别数据
    DEFINE_SERVICE_CONTEXT(RedisDataServiceContext) {
        public:
        std::string id;
    };
};

} // end of namespace
