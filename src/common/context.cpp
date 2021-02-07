#include "common/context.h"
namespace graph_frame {

using namespace google;

Context::Context(const std::string& graph_name):GraphContext(graph_name)
{
}

Context::~Context() 
{
}

int Context::Init()
{
    return 0;
}

} // end of namespace graph_frame
