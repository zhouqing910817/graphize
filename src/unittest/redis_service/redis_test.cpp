#include<gtest/gtest.h>
#include "frame/graph_manager.h"
#include "frame/node_manager.h"
#include "frame/graph.h"
#include "frame/register.h"
#include "common/context.h"
namespace {
class RedisServiceUt:public testing::Test
{
public:
    //添加日志
    static void SetUpTestCase()
    {
	    graph_frame::NodeManager::instance().init("src/unittest/redis_service/conf/node_service.conf");
	    graph_frame::GraphManager::instance().init("src/unittest/redis_service/conf/graph.conf");
    }
    static void TearDownTestCase()
    {
    }
    virtual void SetUp()   //TEST跑之前会执行SetUp
    {
    }
    virtual void TearDown() //TEST跑完之后会执行TearDown
    {
    }
};
TEST_F(RedisServiceUt,test_run)   //此时使用的是TEST_F宏
{
	graph_frame::GraphManager::instance().get_graph("graph1")->run<graph_frame::Context>("graph1");
	sleep(1);
}
}
