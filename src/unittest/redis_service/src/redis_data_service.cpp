#include "unittest/redis_service/src/redis_data_service.h"
#include "frame/register.h"

namespace service {

SERVICE_REGISTER(RedisDataService);

void RedisDataService::init(hocon::shared_config conf) {
	auto fetch_info_obj_list = conf->get_object_list("fetch_infos");
	for (auto fetch_info_obj : fetch_info_obj_list) {
        auto fetch_info_conf = fetch_info_obj->to_config();
	    FetchInfo fetch_info;
        fetch_info.prefix = fetch_info_conf->get_string("prefix");
        fetch_info.postfix = fetch_info_conf->get_string("postfix");
        fetch_info.redis_client_id = fetch_info_conf->get_string("redis_client_id");
        fetch_info.key_gen_func = fetch_info_conf->get_string("key_gen_func");
        fetch_info.response_func = fetch_info_conf->get_string("response_func");
        fetch_info.compress = fetch_info_conf->get_string("compress");
        fetch_info_vec.push_back(std::make_shared<const FetchInfo>(std::move(fetch_info)));
	}
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
