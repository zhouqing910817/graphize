#include "service/base_service/concurrent_service.h"
#include "butil/time.h"
#include "bthread/id.h"
#include "common/context.h"
#include <gflags/gflags.h>
#include "frame/register.h"
#include <functional>
namespace rec {
namespace service {
void ConcurrentService::init(hocon::shared_config config) {

}
size_t ConcurrentService::prepare_sub_requests(std::shared_ptr<graph_frame::Context> context) {
    return 1;
}

void ConcurrentService::process(std::shared_ptr<graph_frame::Context> context, size_t index) {
}

void ConcurrentService::merge(std::shared_ptr<graph_frame::Context> context, size_t slice_num) {
}
struct Mbargs {
    Mbargs (std::function<void(void)> fun) : func(fun){}
    std::function<void(void)> func;
};
static void* m_b_func(void * args_tmp) {
    Mbargs* args = (Mbargs*)args_tmp;
    args->func();
    delete args;
    return nullptr;
}
int ConcurrentService::do_service(std::shared_ptr<graph_frame::Context> context) {
    size_t slice_num = prepare_sub_requests(context);
    std::shared_ptr<std::atomic<int>> count_cnd = std::make_shared<std::atomic<int>>(slice_num);
    for (size_t i = 0; i < slice_num; i++) {
        Mbargs* args = new Mbargs([i, count_cnd, context, slice_num, this](){
            this->process(context, i);
            int old_count = count_cnd->fetch_sub(1);
            if (old_count == 1) {
                this->merge(context, slice_num);
                this->run_output_nodes_if_ready(context);
            }
        });
        bthread_t tid;
        bthread_start_background(&tid, &BTHREAD_ATTR_NORMAL, m_b_func, args);
    }
    return 0;
}
} // end of namespace service
} // end of namespace rec
