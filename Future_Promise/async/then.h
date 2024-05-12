#pragma once

#include "async/future.h"
#include "async/promise.h"

namespace async {

template <typename T, typename C>
auto operator|(Future<T> f, C continuation) {
    return continuation.Pipe(std::move(f));
}

template <typename T, typename C>
auto operator&(Future<T> &f, C continuation) {
    return continuation.And(f);
}

namespace pipe {

template <typename F>
struct [[nodiscard]] Then {
    F cont;

    explicit Then(F continuation) : cont(std::move(continuation)) {}

    // Non-copyable.
    Then(Then&) = delete;

    template <typename T>
    using U = std::invoke_result_t<F, T>;

    template <typename T>
    Future<U<T>> Pipe(Future<T> &&f) {
        return And(f);
    }

    template <typename T>
    Future<U<T>> And(Future<T> &f) {
        Promise<U<T>> p;
        auto cFuture = p.MakeFuture();

        cFuture.SetExecutor(f.GetExecutor());

        f.Then([p = std::move(p), cont = std::move(cont)](_detail::SharedState<T> &state) mutable {
            if (state.exception) {
                std::move(p).SetException(state.exception);
            } else {
                assert(state.result.has_value());

                // Schedule in executor if possible.
                if (state.executor) {
                    state.executor->Submit([value = std::move(state.result.value()),
                                            p = std::move(p),
                                            cont = std::move(cont)]() mutable {
                        try {
                            std::move(p).SetValue(cont(value));
                        } catch(...) {
                            std::move(p).SetException(std::current_exception());
                        }
                    });
                } else {
                    // Otherwise apply continuation immediately.
                    try {
                        std::move(p).SetValue(cont(std::move(state.result.value())));
                    } catch(...) {
                        std::move(p).SetException(std::current_exception());
                    }
                }
            }
        });
        return cFuture;
    }
};

}   // namespace pipe

// Future<T> -> (T -> Result<U>) -> Future<U>
template <typename F>
auto Then(F fun) {
    return pipe::Then{std::move(fun)};
}

}   // namespace async
