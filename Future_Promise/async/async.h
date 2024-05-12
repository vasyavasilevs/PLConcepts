#pragma once

#include <functional>
#include <type_traits>

#include "exec/thread_pool.h"
#include "async/future.h"
#include "async/promise.h"

namespace async {

namespace _detail {

extern exec::ThreadPool _async_pool;

template <class F, class... Args>
using ResultT = std::result_of<typename std::decay<F>::type(
    typename std::decay<Args>::type...)>::type;

}   // namespace _detail

enum class Launch {
    async,
    sync,
};

template <class F, class... Args>
Future<_detail::ResultT<F, Args...>>
Async(Launch policy, F &&func, Args&&... args) {
    using T = _detail::ResultT<F, Args...>;

    if (policy == Launch::async) {
        _detail::_async_pool.Start();
    }

    if (policy == Launch::sync || !_detail::_async_pool.HasFreeWorkers()) {
        try {
            return Future<T>::MakeReady(std::invoke(std::forward<F>(func), std::forward<Args>(args)...));
        } catch(...) {
            return Future<T>::MakeException(std::current_exception());
        }
    }

    Promise<T> p;
    auto f = p.MakeFuture();
    _detail::_async_pool.Submit([p = std::move(p),
                                func = std::forward<F>(func),
                                ... args = std::forward<Args>(args)]() mutable {
        p.SetExecutor(exec::ThreadPool::Current());
        try {
            std::move(p).SetValue(std::invoke(std::move(func), std::move(args)...));
        } catch(...) {
            std::move(p).SetException(std::current_exception());
        }
    });
    return f;
}

template <class F, class... Args>
Future<typename std::result_of<typename std::decay<F>::type(
        typename std::decay<Args>::type...)>::type>
Async(F &&func, Args&&... args) {
    return Async(Launch::async, std::forward<F>(func), std::forward<Args>(args)...);
}

}   // namespace async
