#include "async/async.h"

namespace async::_detail {

exec::ThreadPool _async_pool(std::thread::hardware_concurrency());

}   // namespace async::_detail
