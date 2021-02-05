#include "common/context.h"
namespace graph_frame {
    extern std::vector<std::string> global_channel_name_vec;
}

namespace graph_frame {

using namespace google;

Context::Context()
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
