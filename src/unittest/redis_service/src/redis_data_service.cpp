#include "unittest/redis_service/src/redis_data_service.h"
#include "frame/register.h"

namespace service {

SERVICE_REGISTER(RedisDataService);

void RedisDataService::init(hocon::shared_config conf) {
}

void RedisDataService::process_response(std::shared_ptr<Context> context, const RedisResult& redis_result) {
}


}
