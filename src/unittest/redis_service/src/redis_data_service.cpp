#include "unittest/redis_service/src/redis_data_service.h"
#include "frame/register.h"

namespace service {

SERVICE_REGISTER(RedisDataService);

void RedisDataService::init_redis_conf(hocon::shared_config conf) {
	register_key_func("test_key_gen", [](std::shared_ptr<Context> context,  std::vector<std::string>& keys, const std::string& prefix, const std::string& postfix) -> int {
		LOG(ERROR) << "register key func, prefix: [" << prefix << "]";
		keys.push_back("1");
		keys.push_back("2");
		keys.push_back("3");
		return 1;
	});
	register_parse_func("test_response", [](const std::string& key, const char* result_str, size_t result_str_size, std::shared_ptr<Context> context, std::shared_ptr<const FetchInfo> fetch_info) -> int {
		LOG(ERROR) << "parse_func key:" << key << ", data: " << result_str;
		return 1;
	});
}

void RedisDataService::process_response(std::shared_ptr<Context> context, const RedisResult& redis_result) {
}


}
