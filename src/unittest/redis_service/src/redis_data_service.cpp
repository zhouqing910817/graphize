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

	/*
	 * @param key: redis key
	 * @param result_str: redis value str
	 * @param result_str_size： redis value str
	 * @param slice_index: 访问redis采用拆包方式，包的索引
	 * @param slice_size: 访问redis采用拆包方式，拆分包的总数
	 * @param service_context: 当前service对应的context对象，slice_index slice_size service_context三个参数用于定位当前redis数据应该存放的地方，防止并发解析数据时的数据竞争问题
	 * @param fetch_info: 当前数据对应的fetchinfo，用于打印debug信息
     * @param is_cached: 数据是否是cache 数据
	 */
	register_parse_func("test_response", [](const std::string& key, const char* result_str, size_t result_str_size, int slice_index, int slice_size, std::shared_ptr<graph_frame::GenericServiceContext> context, std::shared_ptr<const FetchInfo> fetch_info, bool is_cached) -> int {
        if (result_str) { 
		    LOG(ERROR) << "parse_func key:" << key << ",is_cached: " << is_cached <<  ", data: " << result_str;
        } else {
		    LOG(ERROR) << "parse_func key:" << key << ",is_cached: " << is_cached <<  ", data: nullptr";
        }
		return 1;
	});
}

void RedisDataService::process_response(std::shared_ptr<Context> context, const RedisResult& redis_result) {
}


}
