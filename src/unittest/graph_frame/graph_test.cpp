#include<gtest/gtest.h>
#include "frame/graph_manager.h"
#include "frame/node_manager.h"
#include "frame/graph.h"
#include "frame/register.h"
#include "common/context.h"
class GraphUt:public testing::Test
{
public:
	static bool init_node_suc;
	static bool init_graph_suc;
    //添加日志
    static void SetUpTestCase()
    {
	    init_node_suc = graph_frame::NodeManager::instance().init("unittest/graph_frame/conf/node_service.conf");
	    init_graph_suc = graph_frame::GraphManager::instance().init("unittest/graph_frame/conf/graph.conf");
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
bool GraphUt::init_node_suc = false;
bool GraphUt::init_graph_suc = false;
TEST_F(GraphUt,test_run_graph)   //此时使用的是TEST_F宏
{
	ASSERT_TRUE(init_node_suc);
	ASSERT_TRUE(init_graph_suc);
	graph_frame::GraphManager::instance().get_graph("graph1")->run<graph_frame::Context>("graph1", nullptr, nullptr,nullptr,nullptr);
	sleep(1);

}
